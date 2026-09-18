/*
 * ACSoap — realtime SOAP command transport (port of AzerothCore mod ACSOAP).
 *
 * tortoise-wow / playerbot-engine-port (2026-09-20). Faithful port of the AC
 * worldserver SOAP layer: an HTTP SOAP endpoint (gSOAP) exposing a single
 * method executeCommand(command) -> result. Commands are queued to the world
 * thread (World::QueueCliCommand) + awaited on a promise/future so the request
 * is realtime request/response (replaces the file-poll agent queue). Auth is
 * account username/password + SEC_ADMINISTRATOR.
 *
 * Adaptations vs AC:
 *   - Print/CommandFinished callbacks take std::any (this core's CliCommandHolder)
 *     instead of void*.
 *   - CliCommandHolder 6-arg ctor (accountId, accessLevel, callbackArg, command,
 *     print, finished).
 *   - Logging via sLog/LOG_DEBUG (this core), World::IsStopped -> sWorld.IsStopped().
 *   - A bind failure logs + disables the service (does NOT World::StopNow, which
 *     is unsafe during init in this core).
 */

#ifndef _TORTOISE_ACSOAP_H
#define _TORTOISE_ACSOAP_H

#include <future>
#include <memory>
#include <string>
#include <any>

void ACSoapThread(std::string const& host, uint16 port);

class SOAPCommand
{
public:
    SOAPCommand() : m_success(false) {}
    ~SOAPCommand() {}

    void appendToPrintBuffer(const char* msg)
    {
        if (msg)
            m_printBuffer += msg;
    }

    void setCommandSuccess(bool val)
    {
        m_success = val;
        try { finishedPromise.set_value(); } catch (...) {}
    }

    bool hasCommandSucceeded() const { return m_success; }

    // CliCommandHolder Print/CommandFinished (this core's signatures).
    static void print(std::any callbackArg, const char* msg)
    {
        SOAPCommand* con = std::any_cast<SOAPCommand*>(callbackArg);
        if (con && msg)
            con->appendToPrintBuffer(msg);
    }

    static void commandFinished(std::any callbackArg, bool success)
    {
        SOAPCommand* con = std::any_cast<SOAPCommand*>(callbackArg);
        if (!con)
            return;
        con->setCommandSuccess(success);
        // world side is done with us; drop the keep-alive (may free the object)
        con->m_self.reset();
    }

    bool m_success;
    std::string m_printBuffer;
    std::promise<void> finishedPromise;
    std::shared_ptr<SOAPCommand> m_self;   // keep-alive (command holds a raw pointer)
};

#endif // _TORTOISE_ACSOAP_H
