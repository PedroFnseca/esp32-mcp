#include "MCPServer.h"
#include "MCPJsonRpc.h"
#include "transports/MCPHttpTransport.h"

MCPServer::MCPServer(const String& name, const String& version)
    : _name(name), _version(version), _protocolVersion("2026-07-28"), _instructions(""), _transport(nullptr), _ownsTransport(false) {
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

String MCPServer::processMessage(const String& message) {
    return MCPJsonRpc::process(message, *this);
}

bool MCPServer::handleServerDiscover(JsonObjectConst params, JsonObject result) {
    (void)params;
    JsonArray supportedVersions = result["supportedVersions"].to<JsonArray>();
    supportedVersions.add(_protocolVersion);

    JsonObject capabilities = result["capabilities"].to<JsonObject>();
    JsonObject toolsCap = capabilities["tools"].to<JsonObject>();
    toolsCap["listChanged"] = false;

    if (_instructions.length() > 0) {
        result["instructions"] = _instructions;
    }

    JsonObject meta = result["_meta"].to<JsonObject>();
    JsonObject sInfo = meta["io.modelcontextprotocol/serverInfo"].to<JsonObject>();
    sInfo["name"] = _name;
    sInfo["version"] = _version;

    JsonObject serverInfo = result["serverInfo"].to<JsonObject>();
    serverInfo["name"] = _name;
    serverInfo["version"] = _version;

    return true;
}

bool MCPServer::handleInitialize(JsonObjectConst params, JsonObject result) {
    (void)params;
    result["protocolVersion"] = _protocolVersion;
    JsonObject capabilities = result["capabilities"].to<JsonObject>();
    JsonObject toolsCap = capabilities["tools"].to<JsonObject>();
    toolsCap["listChanged"] = false;

    JsonObject serverInfo = result["serverInfo"].to<JsonObject>();
    serverInfo["name"] = _name;
    serverInfo["version"] = _version;

    if (_instructions.length() > 0) {
        result["instructions"] = _instructions;
    }
    return true;
}

bool MCPServer::handlePing(JsonObjectConst params, JsonObject result) {
    (void)params;
    (void)result;
    return true;
}

bool MCPServer::handleToolsList(JsonObjectConst params, JsonObject result) {
    (void)params;
    JsonArray toolsArr = result["tools"].to<JsonArray>();
    const std::vector<MCPTool>& tools = _toolRegistry.getTools();

    for (size_t i = 0; i < tools.size(); ++i) {
        JsonObject t = toolsArr.add<JsonObject>();
        t["name"] = tools[i].name;
        t["description"] = tools[i].description;

        if (tools[i].inputSchemaJson.length() > 0) {
            JsonDocument schemaDoc;
            DeserializationError err = deserializeJson(schemaDoc, tools[i].inputSchemaJson);
            if (!err && schemaDoc.is<JsonObject>()) {
                t["inputSchema"] = schemaDoc.as<JsonObject>();
            } else {
                JsonObject schema = t["inputSchema"].to<JsonObject>();
                schema["type"] = "object";
                schema["properties"].to<JsonObject>();
            }
        } else {
            JsonObject schema = t["inputSchema"].to<JsonObject>();
            schema["type"] = "object";
            schema["properties"].to<JsonObject>();
        }
    }
    return true;
}

bool MCPServer::handleToolsCall(JsonObjectConst params, JsonObject result, bool& isProtocolError, int& errorCode, String& errorMessage) {
    if (params["name"].isNull() || !params["name"].is<const char*>()) {
        isProtocolError = true;
        errorCode = MCP_RPC_INVALID_PARAMS;
        errorMessage = "Missing required parameter: name";
        return false;
    }

    String toolName = params["name"].as<const char*>();
    const MCPTool* tool = _toolRegistry.findTool(toolName);
    if (!tool) {
        isProtocolError = true;
        errorCode = MCP_RPC_INVALID_PARAMS;
        errorMessage = "Unknown tool: " + toolName;
        return false;
    }

    JsonObjectConst argsObj = params["arguments"].as<JsonObjectConst>();
    MCPArguments args(argsObj);
    MCPResult toolResult = tool->handler(args);

    JsonArray contentArr = result["content"].to<JsonArray>();
    JsonObject contentItem = contentArr.add<JsonObject>();
    contentItem["type"] = toolResult.contentType.length() > 0 ? toolResult.contentType : "text";
    contentItem["text"] = toolResult.content;

    result["isError"] = toolResult.isError;
    if (toolResult.isError && toolResult.errorCode.length() > 0) {
        result["errorCode"] = toolResult.errorCode;
    }

    return true;
}
