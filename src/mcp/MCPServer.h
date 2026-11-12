#ifndef ESP32_MCP_SERVER_H
#define ESP32_MCP_SERVER_H

#include <Arduino.h>
#include "MCPTypes.h"
#include "MCPTool.h"
#include "MCPToolRegistry.h"
#include "json/MCPJsonConfig.h"
#include "json/MCPJsonScanner.h"
#include "json/MCPJsonWriter.h"
#include "transports/MCPTransport.h"

class MCPServer {
private:
    char _name[32];
    char _version[16];
    char _protocolVersion[16];
    char _instructions[128];
    MCPToolRegistry _toolRegistry;
    MCPTransport* _transport;
    bool _ownsTransport;

    char _requestBuffer[MCP_MAX_REQUEST_SIZE];
    char _responseBuffer[MCP_MAX_RESPONSE_SIZE];
    uint32_t _cacheTtlMs;

public:
    MCPServer(const char* name = "ESP32-MCP", const char* version = "0.0.1");
    MCPServer(const String& name, const String& version = "0.0.1");
    ~MCPServer();

    void setCacheTtl(uint32_t ms);
    uint32_t getCacheTtl() const;

    void setName(const char* name);
    void setName(const String& name);
    const char* getName() const;

    void setVersion(const char* version);
    void setVersion(const String& version);
    const char* getVersion() const;

    void setProtocolVersion(const char* protocolVersion);
    void setProtocolVersion(const String& protocolVersion);
    const char* getProtocolVersion() const;

    void setInstructions(const char* instructions);
    void setInstructions(const String& instructions);
    const char* getInstructions() const;

    MCPToolBuilder tool(const char* name, const char* description);
    MCPToolBuilder tool(const String& name, const String& description);

    bool tool(const char* name, const char* description, MCPToolHandler handler);
    bool tool(const char* name, const char* description, const char* schemaJson, MCPToolHandler handler);
    bool tool(const String& name, const String& description, MCPToolHandler handler);
    bool tool(const String& name, const String& description, const String& schemaJson, MCPToolHandler handler);
    bool tool(const MCPTool& toolObj);

    MCPToolRegistry& getToolRegistry();
    const MCPToolRegistry& getToolRegistry() const;

    bool begin(MCPTransport* transport = nullptr);
    void handleClient();

    size_t processMessage(const char* request, size_t length, char* responseOut, size_t responseCapacity);
    String processMessage(const String& message);

    char* getRequestBuffer() { return _requestBuffer; }
    size_t getRequestBufferSize() const { return sizeof(_requestBuffer); }
    char* getResponseBuffer() { return _responseBuffer; }
    size_t getResponseBufferSize() const { return sizeof(_responseBuffer); }

    bool validateHeaders(const char* protocolVersionHdr, const char* methodHdr, const char* nameHdr, const MCPRpcRequestView& req, char* errOut, size_t errCap);

    bool handleServerDiscover(const MCPRpcRequestView& req, MCPJsonWriter& writer);
    bool handleInitialize(const MCPRpcRequestView& req, MCPJsonWriter& writer);
    bool handlePing(const MCPRpcRequestView& req, MCPJsonWriter& writer);
    bool handleToolsList(const MCPRpcRequestView& req, MCPJsonWriter& writer);
    bool handleToolsCall(const MCPRpcRequestView& req, MCPJsonWriter& writer);
};

#endif
