#ifndef ESP32_MCP_HTTP_TRANSPORT_H
#define ESP32_MCP_HTTP_TRANSPORT_H

#include <Arduino.h>
#include "MCPTransport.h"

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <WebServer.h>
#endif

class MCPHttpTransport : public MCPTransport {
private:
    uint16_t _port;
    String _endpoint;
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    WebServer* _httpServer;
    bool _ownsServer;
#endif

public:
    MCPHttpTransport(uint16_t port = 80, const String& endpoint = "/mcp");
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    MCPHttpTransport(WebServer* existingServer, const String& endpoint = "/mcp");
#endif
    virtual ~MCPHttpTransport();

    virtual bool begin() override;
    virtual void handleClient() override;
    virtual void stop() override;

    uint16_t getPort() const;
    const String& getEndpoint() const;
};

#endif
