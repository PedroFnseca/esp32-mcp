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

    _httpServer->on(_endpoint.c_str(), HTTP_OPTIONS, [this]() {
        _httpServer->sendHeader("Access-Control-Allow-Origin", "*");
        _httpServer->sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        _httpServer->sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
        _httpServer->send(204);
    });

    _httpServer->on(_endpoint.c_str(), HTTP_GET, [this]() {
        _httpServer->sendHeader("Access-Control-Allow-Origin", "*");
        String status = "{\"status\":\"ok\",\"service\":\"ESP32-MCP\"}";
        _httpServer->send(200, "application/json", status);
    });

    _httpServer->on(_endpoint.c_str(), HTTP_POST, [this]() {
        _httpServer->sendHeader("Access-Control-Allow-Origin", "*");
        if (!_server) {
            _httpServer->send(500, "application/json", "{\"error\":\"Server not configured\"}");
            return;
        }

        String requestBody = _httpServer->arg("plain");
        String responseBody = _server->processMessage(requestBody);

        if (responseBody.length() == 0) {
            _httpServer->send(204);
        } else {
            _httpServer->send(200, "application/json", responseBody);
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
