#
# gSOAP — vendored self-contained C/C++ SOAP runtime (from AzerothCore deps/gsoap).
# Compiled as its own static lib WITHOUT the game's PCH so the large self-contained
# stdsoap2.* headers don't interact with the game precompiled header. No OpenSSL
# (WITH_SSL is not defined -> all TLS code is #ifdef'd out).
#

add_library(gsoap STATIC
    stdsoap2.cpp
    soapC.cpp
    soapServer.cpp
)

set_target_properties(gsoap PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(gsoap PROPERTIES FOLDER "Game Libs")

# The gsoap headers (stdsoap2.h / soapH.h / soapStub.h) are self-contained and are
# included by both the gsoap sources and the game's ACSoap.cpp.
target_include_directories(gsoap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
