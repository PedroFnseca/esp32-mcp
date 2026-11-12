#include "MCPServer.h"
#include "MCPJsonRpc.h"
#include "transports/MCPHttpTransport.h"
#include <string.h>
#include <stdio.h>

MCPServer::MCPServer(const String& name, const String& version)
    : _name(name), _version(version), _protocolVersion("2026-07-28"), _instructions(""), _transport(nullptr), _ownsTransport(false) {
    _requestBuffer[0] = '\0';
    _responseBuffer[0] = '\0';
}

MCPServer::~MCPServer() {
    if (_ownsTransport && _transport != nullptr) {
        delete _transport;
        _transport = nullptr;
    }
}

void MCPServer::setName(const String& name) {
    _name = name;
}

const String& MCPServer::getName() const {
    return _name;
}

void MCPServer::setVersion(const String& version) {
    _version = version;
}

const String& MCPServer::getVersion() const {
    return _version;
}

void MCPServer::setProtocolVersion(const String& protocolVersion) {
    _protocolVersion = protocolVersion;
}

const String& MCPServer::getProtocolVersion() const {
    return _protocolVersion;
}

void MCPServer::setInstructions(const String& instructions) {
    _instructions = instructions;
}

const String& MCPServer::getInstructions() const {
    return _instructions;
}

bool MCPServer::tool(const String& name, const String& description, MCPToolHandler handler) {
    return _toolRegistry.registerTool(name, description, handler);
}

bool MCPServer::tool(const String& name, const String& description, const String& schemaJson, MCPToolHandler handler) {
    return _toolRegistry.registerTool(name, description, schemaJson, handler);
}

bool MCPServer::tool(const MCPTool& toolObj) {
    return _toolRegistry.registerTool(toolObj);
}

MCPToolRegistry& MCPServer::getToolRegistry() {
    return _toolRegistry;
}

const MCPToolRegistry& MCPServer::getToolRegistry() const {
    return _toolRegistry;
}

bool MCPServer::begin(MCPTransport* transport) {
    if (transport != nullptr) {
        if (_ownsTransport && _transport != nullptr) {
            delete _transport;
        }
        _transport = transport;
        _ownsTransport = false;
    } else if (_transport == nullptr) {
        _transport = new MCPHttpTransport(80, "/mcp");
        _ownsTransport = true;
    }
    if (_transport != nullptr) {
        _transport->setServer(this);
        return _transport->begin();
    }
    return true;
}

void MCPServer::handleClient() {
    if (_transport != nullptr) {
        _transport->handleClient();
    }
}

size_t MCPServer::processMessage(const char* request, size_t length, char* responseOut, size_t responseCapacity) {
    return MCPJsonRpc::process(request, length, responseOut, responseCapacity, *this);
}

String MCPServer::processMessage(const String& message) {
    size_t len = processMessage(message.c_str(), message.length(), _responseBuffer, sizeof(_responseBuffer));
    if (len == 0) {
        return String("");
    }
    return String(_responseBuffer);
}

bool MCPServer::handleServerDiscover(const MCPRpcRequestView& req, MCPJsonWriter& writer) {
    writer.startObject();
    writer.addString("jsonrpc", "2.0");
    if (req.id.isValid() && !req.id.isNull()) {
        writer.addRaw("id", req.id.ptr, req.id.len);
    } else {
        writer.addNull("id");
    }
    writer.writeKey("result");
    writer.startObject();

    writer.writeKey("supportedVersions");
    writer.startArray();
    writer.writeString(_protocolVersion.c_str());
    writer.endArray();

    writer.writeKey("capabilities");
    writer.startObject();
    writer.writeKey("tools");
    writer.startObject();
    writer.addBool("listChanged", false);
    writer.endObject();
    writer.endObject();

    if (_instructions.length() > 0) {
        writer.addString("instructions", _instructions.c_str());
    }

    writer.writeKey("_meta");
    writer.startObject();
    writer.writeKey("io.modelcontextprotocol/serverInfo");
    writer.startObject();
    writer.addString("name", _name.c_str());
    writer.addString("version", _version.c_str());
    writer.endObject();
    writer.endObject();

    writer.writeKey("serverInfo");
    writer.startObject();
    writer.addString("name", _name.c_str());
    writer.addString("version", _version.c_str());
    writer.endObject();

    writer.endObject();
    writer.endObject();
    return true;
}

bool MCPServer::handleInitialize(const MCPRpcRequestView& req, MCPJsonWriter& writer) {
    writer.startObject();
    writer.addString("jsonrpc", "2.0");
    if (req.id.isValid() && !req.id.isNull()) {
        writer.addRaw("id", req.id.ptr, req.id.len);
    } else {
        writer.addNull("id");
    }
    writer.writeKey("result");
    writer.startObject();

    writer.addString("protocolVersion", _protocolVersion.c_str());

    writer.writeKey("capabilities");
    writer.startObject();
    writer.writeKey("tools");
    writer.startObject();
    writer.addBool("listChanged", false);
    writer.endObject();
    writer.endObject();

    writer.writeKey("serverInfo");
    writer.startObject();
    writer.addString("name", _name.c_str());
    writer.addString("version", _version.c_str());
    writer.endObject();

    if (_instructions.length() > 0) {
        writer.addString("instructions", _instructions.c_str());
    }

    writer.endObject();
    writer.endObject();
    return true;
}

bool MCPServer::handlePing(const MCPRpcRequestView& req, MCPJsonWriter& writer) {
    writer.startObject();
    writer.addString("jsonrpc", "2.0");
    if (req.id.isValid() && !req.id.isNull()) {
        writer.addRaw("id", req.id.ptr, req.id.len);
    } else {
        writer.addNull("id");
    }
    writer.writeKey("result");
    writer.startObject();
    writer.endObject();
    writer.endObject();
    return true;
}

bool MCPServer::handleToolsList(const MCPRpcRequestView& req, MCPJsonWriter& writer) {
    writer.startObject();
    writer.addString("jsonrpc", "2.0");
    if (req.id.isValid() && !req.id.isNull()) {
        writer.addRaw("id", req.id.ptr, req.id.len);
    } else {
        writer.addNull("id");
    }
    writer.writeKey("result");
    writer.startObject();

    writer.writeKey("tools");
    writer.startArray();
    for (size_t i = 0; i < _toolRegistry.count(); ++i) {
        const MCPTool* t = _toolRegistry.getTool(i);
        if (!t) continue;
        writer.startObject();
        writer.addString("name", t->name.c_str());
        writer.addString("description", t->description.c_str());
        if (t->inputSchemaJson.length() > 0) {
            writer.addRaw("inputSchema", t->inputSchemaJson.c_str(), t->inputSchemaJson.length());
        } else {
            writer.addRaw("inputSchema", "{\"type\":\"object\",\"properties\":{}}", 31);
        }
        writer.endObject();
    }
    writer.endArray();

    writer.endObject();
    writer.endObject();
    return true;
}

bool MCPServer::handleToolsCall(const MCPRpcRequestView& req, MCPJsonWriter& writer) {
    if (!req.paramsName.isValid() || !req.paramsName.isString()) {
        MCPJsonRpc::buildErrorResponse(writer, req.id, MCP_RPC_INVALID_PARAMS, "Missing required parameter: name");
        return false;
    }

    char toolName[64];
    if (!req.paramsName.decodeString(toolName, sizeof(toolName))) {
        MCPJsonRpc::buildErrorResponse(writer, req.id, MCP_RPC_INVALID_PARAMS, "Invalid tool name");
        return false;
    }

    const MCPTool* tool = _toolRegistry.findTool(toolName);
    if (!tool) {
        char errMsg[128];
        snprintf(errMsg, sizeof(errMsg), "Unknown tool: %s", toolName);
        MCPJsonRpc::buildErrorResponse(writer, req.id, MCP_RPC_INVALID_PARAMS, errMsg);
        return false;
    }

    MCPArguments args(req.paramsArguments.ptr, req.paramsArguments.len);
    MCPResult res = tool->handler(args);

    writer.startObject();
    writer.addString("jsonrpc", "2.0");
    if (req.id.isValid() && !req.id.isNull()) {
        writer.addRaw("id", req.id.ptr, req.id.len);
    } else {
        writer.addNull("id");
    }
    writer.writeKey("result");
    writer.startObject();

    writer.writeKey("content");
    writer.startArray();
    writer.startObject();
    writer.addString("type", res.contentType.length() > 0 ? res.contentType.c_str() : "text");
    writer.addString("text", res.content.c_str());
    writer.endObject();
    writer.endArray();

    writer.addBool("isError", res.isError);
    if (res.isError && res.errorCode.length() > 0) {
        writer.addString("errorCode", res.errorCode.c_str());
    }

    writer.endObject();
    writer.endObject();
    return true;
}
