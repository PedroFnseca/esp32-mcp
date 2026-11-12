#include "MCPTool.h"
#include "MCPToolRegistry.h"
#include <stdio.h>
#include <string.h>

MCPTool::MCPTool()
    : inputSchemaJson(nullptr), handler(nullptr), fluentHandler(nullptr),
      paramCount(0), isReadOnly(false), isDestructive(false), isIdempotent(false), timeoutMs(0) {
    name[0] = '\0';
    description[0] = '\0';
    title[0] = '\0';
    generatedSchema[0] = '\0';
}

MCPTool::MCPTool(const char* toolName, const char* toolDesc, MCPToolHandler toolHandler)
    : MCPTool() {
    strncpy(name, toolName ? toolName : "", sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    strncpy(description, toolDesc ? toolDesc : "", sizeof(description) - 1);
    description[sizeof(description) - 1] = '\0';
    handler = toolHandler;
}

MCPTool::MCPTool(const char* toolName, const char* toolDesc, const char* schemaJson, MCPToolHandler toolHandler)
    : MCPTool(toolName, toolDesc, toolHandler) {
    inputSchemaJson = schemaJson;
}

MCPTool::MCPTool(const String& toolName, const String& toolDesc, MCPToolHandler toolHandler)
    : MCPTool(toolName.c_str(), toolDesc.c_str(), toolHandler) {
}

MCPTool::MCPTool(const String& toolName, const String& toolDesc, const String& schemaJson, MCPToolHandler toolHandler)
    : MCPTool(toolName.c_str(), toolDesc.c_str(), toolHandler) {
    inputSchemaJson = schemaJson.c_str();
}

void MCPTool::buildGeneratedSchema() {
    MCPJsonWriter writer(generatedSchema, sizeof(generatedSchema));
    writer.startObject();
    writer.addString("type", "object");
    if (title[0] != '\0') {
        writer.addString("title", title);
    }

    writer.writeKey("properties");
    writer.startObject();
    for (size_t i = 0; i < paramCount; ++i) {
        const MCPParamDef& p = params[i];
        writer.writeKey(p.name);
        writer.startObject();
        switch (p.type) {
            case MCPParamType::Boolean:
                writer.addString("type", "boolean");
                break;
            case MCPParamType::Integer:
            case MCPParamType::Long:
                writer.addString("type", "integer");
                if (p.hasMinMax) {
                    writer.addLong("minimum", (long)p.minValue);
                    writer.addLong("maximum", (long)p.maxValue);
                }
                break;
            case MCPParamType::Float:
            case MCPParamType::Double:
                writer.addString("type", "number");
                if (p.hasMinMax) {
                    writer.addDouble("minimum", p.minValue, 2);
                    writer.addDouble("maximum", p.maxValue, 2);
                }
                break;
            case MCPParamType::String:
                writer.addString("type", "string");
                break;
        }
        if (p.description[0] != '\0') {
            writer.addString("description", p.description);
        }
        writer.endObject();
    }
    writer.endObject();

    bool hasRequired = false;
    for (size_t i = 0; i < paramCount; ++i) {
        if (!params[i].isOptional) {
            hasRequired = true;
            break;
        }
    }

    if (hasRequired) {
        writer.writeKey("required");
        writer.startArray();
        for (size_t i = 0; i < paramCount; ++i) {
            if (!params[i].isOptional) {
                writer.writeString(params[i].name);
            }
        }
        writer.endArray();
    }

    writer.endObject();
    inputSchemaJson = generatedSchema;
}

bool MCPTool::validateArgs(const MCPArguments& args, int& errorCode, char* errorMsg, size_t errorMsgCap) const {
    for (size_t i = 0; i < paramCount; ++i) {
        const MCPParamDef& p = params[i];
        if (!args.has(p.name)) {
            if (!p.isOptional) {
                errorCode = MCP_RPC_INVALID_PARAMS;
                snprintf(errorMsg, errorMsgCap, "Missing required parameter: %s", p.name);
                return false;
            }
            continue;
        }

        MCPJsonSlice slice = args.getRaw(p.name);
        switch (p.type) {
            case MCPParamType::Boolean: {
                if (!slice.isBool()) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' must be boolean", p.name);
                    return false;
                }
                if (p.targetPtr) *(bool*)p.targetPtr = slice.asBool();
                break;
            }
            case MCPParamType::Integer: {
                if (!slice.isNumber()) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' must be integer", p.name);
                    return false;
                }
                bool ok = false;
                int val = slice.asInt(0, &ok);
                if (!ok) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' integer overflow", p.name);
                    return false;
                }
                if (p.hasMinMax && (val < p.minValue || val > p.maxValue)) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' out of range", p.name);
                    return false;
                }
                if (p.targetPtr) *(int*)p.targetPtr = val;
                break;
            }
            case MCPParamType::Long: {
                if (!slice.isNumber()) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' must be integer", p.name);
                    return false;
                }
                bool ok = false;
                long val = slice.asLong(0, &ok);
                if (!ok) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' integer overflow", p.name);
                    return false;
                }
                if (p.hasMinMax && (val < p.minValue || val > p.maxValue)) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' out of range", p.name);
                    return false;
                }
                if (p.targetPtr) *(long*)p.targetPtr = val;
                break;
            }
            case MCPParamType::Float: {
                if (!slice.isNumber()) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' must be number", p.name);
                    return false;
                }
                bool ok = false;
                float val = slice.asFloat(0.0f, &ok);
                if (!ok) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' invalid number", p.name);
                    return false;
                }
                if (p.hasMinMax && (val < p.minValue || val > p.maxValue)) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' out of range", p.name);
                    return false;
                }
                if (p.targetPtr) *(float*)p.targetPtr = val;
                break;
            }
            case MCPParamType::Double: {
                if (!slice.isNumber()) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' must be number", p.name);
                    return false;
                }
                bool ok = false;
                double val = slice.asDouble(0.0, &ok);
                if (!ok) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' invalid number", p.name);
                    return false;
                }
                if (p.hasMinMax && (val < p.minValue || val > p.maxValue)) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' out of range", p.name);
                    return false;
                }
                if (p.targetPtr) *(double*)p.targetPtr = val;
                break;
            }
            case MCPParamType::String: {
                if (!slice.isString()) {
                    errorCode = MCP_RPC_INVALID_PARAMS;
                    snprintf(errorMsg, errorMsgCap, "Parameter '%s' must be string", p.name);
                    return false;
                }
                if (p.targetPtr && p.stringCapacity > 0) {
                    slice.decodeString((char*)p.targetPtr, p.stringCapacity);
                }
                break;
            }
        }
    }
    return true;
}

MCPToolBuilder::MCPToolBuilder(MCPToolRegistry* registry, const char* name, const char* description)
    : _registry(registry) {
    strncpy(_tool.name, name ? name : "", sizeof(_tool.name) - 1);
    _tool.name[sizeof(_tool.name) - 1] = '\0';
    strncpy(_tool.description, description ? description : "", sizeof(_tool.description) - 1);
    _tool.description[sizeof(_tool.description) - 1] = '\0';
}

MCPToolBuilder& MCPToolBuilder::param(const char* name, bool* target, const char* desc) {
    if (_tool.paramCount < MCP_MAX_PARAMS) {
        MCPParamDef& p = _tool.params[_tool.paramCount++];
        strncpy(p.name, name ? name : "", sizeof(p.name) - 1);
        p.name[sizeof(p.name) - 1] = '\0';
        strncpy(p.description, desc ? desc : "", sizeof(p.description) - 1);
        p.description[sizeof(p.description) - 1] = '\0';
        p.type = MCPParamType::Boolean;
        p.targetPtr = target;
        p.isOptional = false;
        p.hasMinMax = false;
    }
    return *this;
}

MCPToolBuilder& MCPToolBuilder::param(const char* name, int* target, const char* desc) {
    if (_tool.paramCount < MCP_MAX_PARAMS) {
        MCPParamDef& p = _tool.params[_tool.paramCount++];
        strncpy(p.name, name ? name : "", sizeof(p.name) - 1);
        p.name[sizeof(p.name) - 1] = '\0';
        strncpy(p.description, desc ? desc : "", sizeof(p.description) - 1);
        p.description[sizeof(p.description) - 1] = '\0';
        p.type = MCPParamType::Integer;
        p.targetPtr = target;
        p.isOptional = false;
        p.hasMinMax = false;
    }
    return *this;
}

MCPToolBuilder& MCPToolBuilder::param(const char* name, int* target, int minVal, int maxVal, const char* desc) {
    if (_tool.paramCount < MCP_MAX_PARAMS) {
        MCPParamDef& p = _tool.params[_tool.paramCount++];
        strncpy(p.name, name ? name : "", sizeof(p.name) - 1);
        p.name[sizeof(p.name) - 1] = '\0';
        strncpy(p.description, desc ? desc : "", sizeof(p.description) - 1);
        p.description[sizeof(p.description) - 1] = '\0';
        p.type = MCPParamType::Integer;
        p.targetPtr = target;
        p.isOptional = false;
        p.hasMinMax = true;
        p.minValue = minVal;
        p.maxValue = maxVal;
    }
    return *this;
}

MCPToolBuilder& MCPToolBuilder::param(const char* name, long* target, const char* desc) {
    if (_tool.paramCount < MCP_MAX_PARAMS) {
        MCPParamDef& p = _tool.params[_tool.paramCount++];
        strncpy(p.name, name ? name : "", sizeof(p.name) - 1);
        p.name[sizeof(p.name) - 1] = '\0';
        strncpy(p.description, desc ? desc : "", sizeof(p.description) - 1);
        p.description[sizeof(p.description) - 1] = '\0';
        p.type = MCPParamType::Long;
        p.targetPtr = target;
        p.isOptional = false;
        p.hasMinMax = false;
    }
    return *this;
}

MCPToolBuilder& MCPToolBuilder::param(const char* name, long* target, long minVal, long maxVal, const char* desc) {
    if (_tool.paramCount < MCP_MAX_PARAMS) {
        MCPParamDef& p = _tool.params[_tool.paramCount++];
        strncpy(p.name, name ? name : "", sizeof(p.name) - 1);
        p.name[sizeof(p.name) - 1] = '\0';
        strncpy(p.description, desc ? desc : "", sizeof(p.description) - 1);
        p.description[sizeof(p.description) - 1] = '\0';
        p.type = MCPParamType::Long;
        p.targetPtr = target;
        p.isOptional = false;
        p.hasMinMax = true;
        p.minValue = minVal;
        p.maxValue = maxVal;
    }
    return *this;
}

MCPToolBuilder& MCPToolBuilder::param(const char* name, float* target, const char* desc) {
    if (_tool.paramCount < MCP_MAX_PARAMS) {
        MCPParamDef& p = _tool.params[_tool.paramCount++];
        strncpy(p.name, name ? name : "", sizeof(p.name) - 1);
        p.name[sizeof(p.name) - 1] = '\0';
        strncpy(p.description, desc ? desc : "", sizeof(p.description) - 1);
        p.description[sizeof(p.description) - 1] = '\0';
        p.type = MCPParamType::Float;
        p.targetPtr = target;
        p.isOptional = false;
        p.hasMinMax = false;
    }
    return *this;
}

MCPToolBuilder& MCPToolBuilder::param(const char* name, float* target, float minVal, float maxVal, const char* desc) {
    if (_tool.paramCount < MCP_MAX_PARAMS) {
        MCPParamDef& p = _tool.params[_tool.paramCount++];
        strncpy(p.name, name ? name : "", sizeof(p.name) - 1);
        p.name[sizeof(p.name) - 1] = '\0';
        strncpy(p.description, desc ? desc : "", sizeof(p.description) - 1);
        p.description[sizeof(p.description) - 1] = '\0';
        p.type = MCPParamType::Float;
        p.targetPtr = target;
        p.isOptional = false;
        p.hasMinMax = true;
        p.minValue = minVal;
        p.maxValue = maxVal;
    }
    return *this;
}

MCPToolBuilder& MCPToolBuilder::param(const char* name, double* target, const char* desc) {
    if (_tool.paramCount < MCP_MAX_PARAMS) {
        MCPParamDef& p = _tool.params[_tool.paramCount++];
        strncpy(p.name, name ? name : "", sizeof(p.name) - 1);
        p.name[sizeof(p.name) - 1] = '\0';
        strncpy(p.description, desc ? desc : "", sizeof(p.description) - 1);
        p.description[sizeof(p.description) - 1] = '\0';
        p.type = MCPParamType::Double;
        p.targetPtr = target;
        p.isOptional = false;
        p.hasMinMax = false;
    }
    return *this;
}

MCPToolBuilder& MCPToolBuilder::param(const char* name, double* target, double minVal, double maxVal, const char* desc) {
    if (_tool.paramCount < MCP_MAX_PARAMS) {
        MCPParamDef& p = _tool.params[_tool.paramCount++];
        strncpy(p.name, name ? name : "", sizeof(p.name) - 1);
        p.name[sizeof(p.name) - 1] = '\0';
        strncpy(p.description, desc ? desc : "", sizeof(p.description) - 1);
        p.description[sizeof(p.description) - 1] = '\0';
        p.type = MCPParamType::Double;
        p.targetPtr = target;
        p.isOptional = false;
        p.hasMinMax = true;
        p.minValue = minVal;
        p.maxValue = maxVal;
    }
    return *this;
}

MCPToolBuilder& MCPToolBuilder::param(const char* name, char* target, size_t capacity, const char* desc) {
    if (_tool.paramCount < MCP_MAX_PARAMS) {
        MCPParamDef& p = _tool.params[_tool.paramCount++];
        strncpy(p.name, name ? name : "", sizeof(p.name) - 1);
        p.name[sizeof(p.name) - 1] = '\0';
        strncpy(p.description, desc ? desc : "", sizeof(p.description) - 1);
        p.description[sizeof(p.description) - 1] = '\0';
        p.type = MCPParamType::String;
        p.targetPtr = target;
        p.stringCapacity = capacity;
        p.isOptional = false;
        p.hasMinMax = false;
    }
    return *this;
}

MCPToolBuilder& MCPToolBuilder::optional() {
    if (_tool.paramCount > 0) {
        _tool.params[_tool.paramCount - 1].isOptional = true;
    }
    return *this;
}

MCPToolBuilder& MCPToolBuilder::title(const char* t) {
    strncpy(_tool.title, t ? t : "", sizeof(_tool.title) - 1);
    _tool.title[sizeof(_tool.title) - 1] = '\0';
    return *this;
}

MCPToolBuilder& MCPToolBuilder::readOnly() {
    _tool.isReadOnly = true;
    return *this;
}

MCPToolBuilder& MCPToolBuilder::destructive() {
    _tool.isDestructive = true;
    return *this;
}

MCPToolBuilder& MCPToolBuilder::idempotent() {
    _tool.isIdempotent = true;
    return *this;
}

MCPToolBuilder& MCPToolBuilder::timeout(uint32_t ms) {
    _tool.timeoutMs = ms;
    return *this;
}

bool MCPToolBuilder::handle(MCPFluentHandler callback) {
    _tool.fluentHandler = callback;
    _tool.buildGeneratedSchema();
    if (_registry) {
        return _registry->registerTool(_tool);
    }
    return false;
}

bool MCPToolBuilder::handle(MCPToolHandler callback) {
    _tool.handler = callback;
    _tool.buildGeneratedSchema();
    if (_registry) {
        return _registry->registerTool(_tool);
    }
    return false;
}
