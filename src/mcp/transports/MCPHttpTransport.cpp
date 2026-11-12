#include "MCPHttpTransport.h"
#include "../MCPServer.h"

MCPHttpTransport::MCPHttpTransport(uint16_t port, const String& endpoint)
    : _port(port), _endpoint(endpoint)
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    , _httpServer(nullptr), _ownsServer(true)
#endif
{
}

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
MCPHttpTransport::MCPHttpTransport(WebServer* existingServer, const String& endpoint)
    : _port(80), _endpoint(endpoint), _httpServer(existingServer), _ownsServer(false)
{
}
#endif

MCPHttpTransport::~MCPHttpTransport() {
    stop();
}

bool MCPHttpTransport::begin() {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    if (_httpServer == nullptr && _ownsServer) {
        _httpServer = new WebServer(_port);
    }

    if (_httpServer == nullptr) {
        return false;
    }

    const char* HEADER_KEYS[] = {
        "MCP-Protocol-Version",
        "Mcp-Method",
        "Mcp-Name",
        "Authorization",
        "Origin"
    };
    _httpServer->collectHeaders(HEADER_KEYS, sizeof(HEADER_KEYS) / sizeof(HEADER_KEYS[0]));

    _httpServer->on(_endpoint.c_str(), HTTP_OPTIONS, [this]() {
        String originHdr = _httpServer->header("Origin");
        if (_server && _server->checkOrigin(originHdr.c_str())) {
            _httpServer->sendHeader("Access-Control-Allow-Origin", _server->getAllowedOrigin());
            _httpServer->sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
            _httpServer->sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, MCP-Protocol-Version, Mcp-Method, Mcp-Name");
            _httpServer->send(204);
        } else {
            _httpServer->send(403, "application/json", "{\"error\":\"CORS origin forbidden\"}");
        }
    });

    _httpServer->on(_endpoint.c_str(), HTTP_GET, [this]() {
        String originHdr = _httpServer->header("Origin");
        if (_server && _server->checkOrigin(originHdr.c_str())) {
            _httpServer->sendHeader("Access-Control-Allow-Origin", _server->getAllowedOrigin());
        }
        String status = "{\"status\":\"ok\",\"service\":\"ESP32-MCP\"}";
        _httpServer->send(200, "application/json", status);
    });

    _httpServer->on(_endpoint.c_str(), HTTP_POST, [this]() {
        if (!_server) {
            _httpServer->send(500, "application/json", "{\"error\":\"Server not configured\"}");
            return;
        }

        String originHdr = _httpServer->header("Origin");
        if (_server->checkOrigin(originHdr.c_str())) {
            _httpServer->sendHeader("Access-Control-Allow-Origin", _server->getAllowedOrigin());
        }

        String authHdr = _httpServer->header("Authorization");
        if (!_server->checkBearer(authHdr.c_str())) {
            _httpServer->send(401, "application/json", "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32600,\"message\":\"Unauthorized\"}}");
            return;
        }

        const String& requestBody = _httpServer->arg("plain");
        if (requestBody.length() > MCP_MAX_REQUEST_SIZE) {
            _httpServer->send(413, "application/json", "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32600,\"message\":\"Payload too large\"}}");
            return;
        }

        String protoHdr = _httpServer->header("MCP-Protocol-Version");
        String methodHdr = _httpServer->header("Mcp-Method");
        String nameHdr = _httpServer->header("Mcp-Name");

        MCPRpcRequestView reqView;
        MCPScanError scanErr = MCPScanError::None;
        if (MCPJsonScanner::parseRpcRequest(requestBody.c_str(), requestBody.length(), reqView, &scanErr)) {
            char hdrErr[128] = {0};
            if (!_server->validateHeaders(protoHdr.c_str(), methodHdr.c_str(), nameHdr.c_str(), reqView, hdrErr, sizeof(hdrErr))) {
                _httpServer->send(400, "application/json", hdrErr);
                return;
            }
        }

        size_t respLen = _server->processMessage(requestBody.c_str(), requestBody.length(), _server->getResponseBuffer(), _server->getResponseBufferSize());

        if (respLen == 0) {
            _httpServer->send(204);
        } else {
            _httpServer->send(200, "application/json", _server->getResponseBuffer());
        }
    });

    if (_ownsServer) {
        _httpServer->begin();
    }
    return true;
#else
    return true;
#endif
}

void MCPHttpTransport::handleClient() {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    if (_httpServer != nullptr && _ownsServer) {
        _httpServer->handleClient();
    }
#endif
}

void MCPHttpTransport::stop() {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    if (_ownsServer && _httpServer != nullptr) {
        _httpServer->stop();
        delete _httpServer;
        _httpServer = nullptr;
    }
#endif
}

uint16_t MCPHttpTransport::getPort() const {
    return _port;
}

const String& MCPHttpTransport::getEndpoint() const {
    return _endpoint;
}
