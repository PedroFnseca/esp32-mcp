#include "MCPServer.h"
#include "MCPJsonRpc.h"
#include "transports/MCPHttpTransport.h"
#include <string.h>
#include <stdio.h>

MCPServer::MCPServer(const char* name, const char* version)
    : _transport(nullptr), _ownsTransport(false) {
    setName(name ? name : "ESP32-MCP");
    setVersion(version ? version : "0.0.1");
    setProtocolVersion("2026-07-28");
    _instructions[0] = '\0';
    _requestBuffer[0] = '\0';
    _responseBuffer[0] = '\0';
}

MCPServer::MCPServer(const String& name, const String& version)
    : MCPServer(name.c_str(), version.c_str()) {
}

MCPServer::~MCPServer() {
    if (_ownsTransport && _transport != nullptr) {
        delete _transport;
        _transport = nullptr;
    }
}

void MCPServer::setName(const char* name) {
    strncpy(_name, name ? name : "", sizeof(_name) - 1);
    _name[sizeof(_name) - 1] = '\0';
}

void MCPServer::setName(const String& name) {
    setName(name.c_str());
}

const char* MCPServer::getName() const {
    return _name;
}

void MCPServer::setVersion(const char* version) {
    strncpy(_version, version ? version : "", sizeof(_version) - 1);
    _version[sizeof(_version) - 1] = '\0';
}

void MCPServer::setVersion(const String& version) {
    setVersion(version.c_str());
}

const char* MCPServer::getVersion() const {
    return _version;
}

void MCPServer::setProtocolVersion(const char* protocolVersion) {
    strncpy(_protocolVersion, protocolVersion ? protocolVersion : "", sizeof(_protocolVersion) - 1);
    _protocolVersion[sizeof(_protocolVersion) - 1] = '\0';
}

void MCPServer::setProtocolVersion(const String& protocolVersion) {
    setProtocolVersion(protocolVersion.c_str());
}

const char* MCPServer::getProtocolVersion() const {
    return _protocolVersion;
}

void MCPServer::setInstructions(const char* instructions) {
    strncpy(_instructions, instructions ? instructions : "", sizeof(_instructions) - 1);
    _instructions[sizeof(_instructions) - 1] = '\0';
}

void MCPServer::setInstructions(const String& instructions) {
    setInstructions(instructions.c_str());
}

const char* MCPServer::getInstructions() const {
    return _instructions;
}

MCPToolBuilder MCPServer::tool(const char* name, const char* description) {
    return MCPToolBuilder(&_toolRegistry, name, description);
}

MCPToolBuilder MCPServer::tool(const String& name, const String& description) {
    return tool(name.c_str(), description.c_str());
}

bool MCPServer::tool(const char* name, const char* description, MCPToolHandler handler) {
    return _toolRegistry.registerTool(name, description, handler);
}

bool MCPServer::tool(const char* name, const char* description, const char* schemaJson, MCPToolHandler handler) {
    return _toolRegistry.registerTool(name, description, schemaJson, handler);
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
    writer.writeString(_protocolVersion);
    writer.endArray();

    writer.writeKey("capabilities");
    writer.startObject();
    writer.writeKey("tools");
    writer.startObject();
    writer.addBool("listChanged", false);
    writer.endObject();
    writer.endObject();

    if (_instructions[0] != '\0') {
        writer.addString("instructions", _instructions);
    }

    writer.writeKey("_meta");
    writer.startObject();
    writer.writeKey("io.modelcontextprotocol/serverInfo");
    writer.startObject();
    writer.addString("name", _name);
    writer.addString("version", _version);
    writer.endObject();
    writer.endObject();

    writer.writeKey("serverInfo");
    writer.startObject();
    writer.addString("name", _name);
    writer.addString("version", _version);
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

    writer.addString("protocolVersion", _protocolVersion);

    writer.writeKey("capabilities");
    writer.startObject();
    writer.writeKey("tools");
    writer.startObject();
    writer.addBool("listChanged", false);
    writer.endObject();
    writer.endObject();

    writer.writeKey("serverInfo");
    writer.startObject();
    writer.addString("name", _name);
    writer.addString("version", _version);
    writer.endObject();

    if (_instructions[0] != '\0') {
        writer.addString("instructions", _instructions);
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
        writer.addString("name", t->name);
        writer.addString("description", t->description);
        if (t->title[0] != '\0') {
            writer.addString("title", t->title);
        }
        if (t->isReadOnly || t->isDestructive || t->isIdempotent) {
            writer.writeKey("annotations");
            writer.startObject();
            if (t->isReadOnly) writer.addBool("readOnly", true);
            if (t->isDestructive) writer.addBool("destructive", true);
            if (t->isIdempotent) writer.addBool("idempotent", true);
            writer.endObject();
        }
        if (t->inputSchemaJson && t->inputSchemaJson[0] != '\0') {
            writer.addRaw("inputSchema", t->inputSchemaJson, strlen(t->inputSchemaJson));
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

    int protoErrCode = 0;
    char protoErrMsg[128];
    if (!tool->validateArgs(args, protoErrCode, protoErrMsg, sizeof(protoErrMsg))) {
        MCPJsonRpc::buildErrorResponse(writer, req.id, protoErrCode, protoErrMsg);
        return false;
    }

    MCPResult res;
    if (tool->fluentHandler) {
        tool->fluentHandler(res);
    } else if (tool->handler) {
        res = tool->handler(args);
    }

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
    writer.addString("type", res.contentType()[0] != '\0' ? res.contentType() : "text");
    writer.addString("text", res.content());
    writer.endObject();
    writer.endArray();

    writer.addBool("isError", res.isError);
    if (res.isError && res.errorCode()[0] != '\0') {
        writer.addString("errorCode", res.errorCode());
    }

    writer.endObject();
    writer.endObject();
    return true;
}
