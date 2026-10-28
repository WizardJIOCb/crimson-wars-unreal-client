// IXWebSocket Library Implementation Wrapper
// This file includes all IXWebSocket source files for compilation

// Disable warnings for third-party library
#pragma warning(push, 0)
#pragma warning(disable: 4456) // declaration hides previous local declaration

// Fix Windows.h min/max macro conflict
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Undefine Windows macros that conflict with IXWebSocket
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

// Platform-specific socket implementations
#if PLATFORM_WINDOWS
	#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXSocket.cpp"
	#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXSelectInterruptPipe.cpp"
#endif

// Core WebSocket implementation
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXWebSocket.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXWebSocketServer.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXWebSocketTransport.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXWebSocketHandshake.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXWebSocketHttpHeaders.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXWebSocketPerMessageDeflate.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXWebSocketPerMessageDeflateCodec.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXWebSocketPerMessageDeflateOptions.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXWebSocketCloseConstants.cpp"

// Socket server implementation
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXSocketServer.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXSocketConnect.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXSocketFactory.cpp"

// Utilities
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXCancellationRequest.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXConnectionState.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXUrlParser.cpp"
// IXDNSLookup.cpp needs gai_strerrorA (char*), but the PCH includes ws2tcpip.h
// in UNICODE mode which maps gai_strerror -> gai_strerrorW (WCHAR*).
// We must explicitly override the macro before inclusion and restore after.
#pragma push_macro("UNICODE")
#pragma push_macro("_UNICODE")
#pragma push_macro("gai_strerror")
#undef UNICODE
#undef _UNICODE
#undef gai_strerror
#define gai_strerror gai_strerrorA
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXDNSLookup.cpp"
#pragma pop_macro("gai_strerror")
#pragma pop_macro("_UNICODE")
#pragma pop_macro("UNICODE")
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXUuid.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXSetThreadName.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXNetSystem.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXUserAgent.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXGzipCodec.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXSelectInterrupt.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXSelectInterruptFactory.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXSelectInterruptEvent.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXStrCaseCompare.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXExponentialBackoff.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXHttp.cpp"
#include "../../../ThirdParty/IXWebSocket/ixwebsocket/IXHttpClient.cpp"

#pragma warning(pop)

