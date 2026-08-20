#ifndef ESP32_MCP_TRANSPORT_H
#define ESP32_MCP_TRANSPORT_H

#include <Arduino.h>

class MCPServer;

class MCPTransport {
protected:
    MCPServer* _server;

public:
    MCPTransport() : _server(nullptr) {}
    virtual ~MCPTransport() {}

    virtual void setServer(MCPServer* server) {
        _server = server;
    }

    virtual bool begin() = 0;
    virtual void handleClient() = 0;
    virtual void stop() {}
};

#endif
