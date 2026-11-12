#include "MCPServer.h"
#include "MCPJsonRpc.h"
#include "transports/MCPHttpTransport.h"
#include <string.h>
#include <stdio.h>

MCPServer::MCPServer(const char* name, const char* version)
    : _transport(nullptr), _ownsTransport(false), _cacheTtlMs(60000),
      _hasBearer(false), _authWarned(false), _useBackgroundTask(true) {
    setName(name ? name : "ESP32-MCP");
    setVersion(version ? version : "0.0.1");
    setProtocolVersion("2026-07-28");
    _instructions[0] = '\0';
    _requestBuffer[0] = '\0';
    _responseBuffer[0] = '\0';
    _bearerToken[0] = '\0';
    _allowedOrigin[0] = '\0';
}

void MCPServer::bearer(const char* token) {
    if (token && token[0] != '\0') {
        strncpy(_bearerToken, token, sizeof(_bearerToken) - 1);
        _bearerToken[sizeof(_bearerToken) - 1] = '\0';
        _hasBearer = true;
    } else {
        _bearerToken[0] = '\0';
        _hasBearer = false;
    }
}

void MCPServer::bearer(const String& token) {
    bearer(token.c_str());
}

void MCPServer::allowOrigin(const char* origin) {
    if (origin && origin[0] != '\0') {
        strncpy(_allowedOrigin, origin, sizeof(_allowedOrigin) - 1);
        _allowedOrigin[sizeof(_allowedOrigin) - 1] = '\0';
    } else {
        _allowedOrigin[0] = '\0';
    }
}

void MCPServer::allowOrigin(const String& origin) {
    allowOrigin(origin.c_str());
}

static bool constantTimeCompare(const char* a, const char* b) {
    if (!a || !b) return false;
    size_t lenA = strlen(a);
    size_t lenB = strlen(b);
    if (lenA != lenB) return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < lenA; ++i) {
        diff |= ((unsigned char)a[i] ^ (unsigned char)b[i]);
    }
    return diff == 0;
}

bool MCPServer::checkBearer(const char* authHeader) {
    if (!_hasBearer) return true;
    if (!authHeader) return false;
    if (strncmp(authHeader, "Bearer ", 7) != 0) return false;
    return constantTimeCompare(authHeader + 7, _bearerToken);
}

bool MCPServer::checkOrigin(const char* originHeader) {
    if (_allowedOrigin[0] == '\0') return false;
    if (!originHeader) return false;
    return strcmp(originHeader, _allowedOrigin) == 0;
}

void MCPServer::checkAndWarnAuth() {
    if (!_hasBearer && !_authWarned) {
        _authWarned = true;
        Serial.println("[ESP32-MCP] WARNING: Server running with NO authentication (bearer token not set). Accessible to anyone on the network!");
    }
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

void MCPServer::setCacheTtl(uint32_t ms) {
    _cacheTtlMs = ms;
}

uint32_t MCPServer::getCacheTtl() const {
    return _cacheTtlMs;
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
    checkAndWarnAuth();

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
        bool ok = _transport->begin();
        if (!ok) return false;

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
        if (_useBackgroundTask) {
            xTaskCreatePinnedToCore(
                [](void* param) {
                    MCPServer* s = (MCPServer*)param;
                    for (;;) {
                        s->handleClient();
                        vTaskDelay(pdMS_TO_TICKS(5));
                    }
                },
                "mcp_server",
                4096,
                this,
                1,
                nullptr,
                1
            );
        }
#endif
        return true;
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

bool MCPServer::validateHeaders(const char* protocolVersionHdr, const char* methodHdr, const char* nameHdr, const MCPRpcRequestView& req, char* errOut, size_t errCap) {
    if (protocolVersionHdr && protocolVersionHdr[0] != '\0') {
        if (strcmp(protocolVersionHdr, "2026-07-28") != 0 && strcmp(protocolVersionHdr, "2024-11-05") != 0) {
            snprintf(errOut, errCap, "Unsupported MCP-Protocol-Version: %s", protocolVersionHdr);
            return false;
        }
    }

    if (methodHdr && methodHdr[0] != '\0') {
        char bodyMethod[64] = {0};
        req.method.decodeString(bodyMethod, sizeof(bodyMethod));
        if (strcmp(methodHdr, bodyMethod) != 0) {
            snprintf(errOut, errCap, "Mcp-Method header '%s' does not match body method '%s'", methodHdr, bodyMethod);
            return false;
        }
    }

    if (nameHdr && nameHdr[0] != '\0') {
        char bodyName[64] = {0};
        req.paramsName.decodeString(bodyName, sizeof(bodyName));
        if (strcmp(nameHdr, bodyName) != 0) {
            snprintf(errOut, errCap, "Mcp-Name header '%s' does not match tool name '%s'", nameHdr, bodyName);
            return false;
        }
    }

    return true;
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
    writer.writeString("2026-07-28");
    writer.writeString("2024-11-05");
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
    writer.addLong("ttlMs", (long)_cacheTtlMs);
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

    const char* respVersion = _protocolVersion;
    if (req.params.isValid() && req.params.isObject()) {
        MCPJsonSlice clientVerSlice = MCPJsonScanner::findKey(req.params.ptr, req.params.len, "protocolVersion");
        if (clientVerSlice.isValid() && clientVerSlice.isString()) {
            if (clientVerSlice.equalsString("2024-11-05")) {
                respVersion = "2024-11-05";
            }
        }
    }

    writer.addString("protocolVersion", respVersion);

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

    writer.writeKey("_meta");
    writer.startObject();
    writer.addLong("ttlMs", (long)_cacheTtlMs);
    writer.endObject();

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
