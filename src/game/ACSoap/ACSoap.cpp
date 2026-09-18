/*
 * ACSoap — realtime SOAP command transport (port of AzerothCore mod ACSOAP).
 * See ACSoap.h for the design + adaptations.
 */

#include "ACSoap.h"
#include "AccountMgr.h"
#include "Log.h"
#include "Logging.h"
#include "World.h"
#include "Config/Config.h"
#include "soapStub.h"
#include <chrono>
#include <memory>

void process_message(struct soap* soap_message);

void ACSoapThread(std::string const& host, uint16 port)
{
    struct soap soap;
    soap_init(&soap);
    soap_set_imode(&soap, SOAP_C_UTFSTRING);
    soap_set_omode(&soap, SOAP_C_UTFSTRING);

    // check every 3 seconds if world ended
    soap.accept_timeout = 3;
    soap.recv_timeout = 5;
    soap.send_timeout = 5;

    // allow rebinding while the previous socket is still in TIME_WAIT
    soap.bind_flags = SO_REUSEADDR;

    if (!soap_valid_socket(soap_bind(&soap, host.c_str(), port, 100)))
    {
        sLog.outError("playerbots: ACSoap: couldn't bind to %s:%u (SOAP disabled)", host.c_str(), port);
        soap_destroy(&soap);
        soap_end(&soap);
        soap_done(&soap);
        return;
    }

    sLog.outInfo("playerbots: ACSoap: bound to http://%s:%u (executeCommand; auth = account username/password + rank >= SOAP.MinRank)",
        host.c_str(), port);

    while (!sWorld.IsStopped())
    {
        if (!soap_valid_socket(soap_accept(&soap)))
            continue;   // ran into an accept timeout

        LOG_DEBUG("playerbots", "ACSoap: accepted connection");
        struct soap* thread_soap = soap_copy(&soap);   // make a safe copy
        process_message(thread_soap);
    }

    soap_destroy(&soap);
    soap_end(&soap);
    soap_done(&soap);
}

void process_message(struct soap* soap_message)
{
    soap_serve(soap_message);
    soap_destroy(soap_message); // dealloc C++ data
    soap_end(soap_message);     // dealloc data and clean up
    soap_free(soap_message);    // detach soap struct and free up the memory
}

/*
    Generated stub signature (gsoap.stub):
        int ns1__executeCommand(char* command, char** result);
    gSOAP invokes it as:
        int ns1__executeCommand(soap* soap, char* command, char** result);
*/
int ns1__executeCommand(soap* soap, char* command, char** result)
{
    // security check
    if (!soap->userid || !soap->passwd)
    {
        LOG_DEBUG("playerbots", "ACSoap: client didn't provide login information");
        return 401;
    }

    uint32 accountId = sAccountMgr.GetId(soap->userid);
    if (!accountId)
    {
        LOG_DEBUG("playerbots", "ACSoap: client used unknown username '%s'", soap->userid);
        return 401;
    }

    if (!sAccountMgr.CheckPassword(accountId, soap->passwd, soap->userid))
    {
        LOG_DEBUG("playerbots", "ACSoap: invalid password for account '%s'", soap->userid);
        return 401;
    }

    int32 minRank = sConfig.GetIntDefault("SOAP.MinRank", 3);   // default SEC_DEVELOPER (admin rank)
    if (sAccountMgr.GetSecurity(accountId) < (AccountTypes)minRank)
    {
        LOG_DEBUG("playerbots", "ACSoap: %s's rank (%d) is too low (need >= %d)", soap->userid,
            (int)sAccountMgr.GetSecurity(accountId), minRank);
        return 403;
    }

    if (!command || !*command)
        return soap_sender_fault(soap, "Command can not be empty", "The supplied command was an empty string");

    LOG_DEBUG("playerbots", "ACSoap: got command '%s' from '%s'", command, soap->userid);

    // Shared so the object survives if we stop waiting below: the queued command keeps a raw
    // pointer to it and the world thread may still run it after that. The extra reference is
    // released by commandFinished() once the world side is done with it.
    std::shared_ptr<SOAPCommand> connection = std::make_shared<SOAPCommand>();
    connection->m_self = connection;

    // commands are executed in the world thread. We have to wait for them to be completed.
    // Run the command at the authenticated account's own security level (no escalation).
    sWorld.QueueCliCommand(new CliCommandHolder(
        accountId,
        sAccountMgr.GetSecurity(accountId),
        std::any(connection.get()),
        command,
        &SOAPCommand::print,
        &SOAPCommand::commandFinished));

    // Wait for the command to finish, but bail on shutdown: ProcessCliCommands() (which
    // fulfils the promise) stops once the world loop exits, so an unbounded wait here would deadlock.
    std::future<void> finished = connection->finishedPromise.get_future();
    while (finished.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
    {
        if (sWorld.IsStopped())
            return soap_receiver_fault(soap, "Server is shutting down", "Command aborted: the server is shutting down");
    }

    // The command has finished executing already
    char* printBuffer = soap_strdup(soap, connection->m_printBuffer.c_str());
    if (connection->hasCommandSucceeded())
    {
        *result = printBuffer;
        return SOAP_OK;
    }
    else
        return soap_sender_fault(soap, printBuffer, printBuffer);
}

////////////////////////////////////////////////////////////////////////////////
//
//  Namespace Definition Table
//
////////////////////////////////////////////////////////////////////////////////

struct Namespace namespaces[] =
{
    { "SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", nullptr, nullptr }, // must be first
    { "SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", nullptr, nullptr }, // must be second
    { "xsi", "http://www.w3.org/1999/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", nullptr },
    { "xsd", "http://www.w3.org/1999/XMLSchema",          "http://www.w3.org/*/XMLSchema", nullptr },
    { "ns1", "urn:AC", nullptr, nullptr },     // "ns1" namespace prefix
    { nullptr, nullptr, nullptr, nullptr }
};
