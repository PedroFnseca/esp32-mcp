#ifndef ESP32_MCP_TOOL_H
#define ESP32_MCP_TOOL_H

#include <Arduino.h>
#include <functional>
#include <string.h>
#include "MCPTypes.h"
#include "json/MCPJsonConfig.h"

typedef std::function<MCPResult(const MCPArguments&)> MCPToolHandler;
typedef std::function<void(MCPResult&)> MCPFluentHandler;

enum class MCPParamType {
    Boolean,
    Integer,
    Long,
    Float,
    Double,
    String
};

struct MCPParamDef {
    char name[32];
    char description[64];
    MCPParamType type;
    void* targetPtr;
    size_t stringCapacity;
    bool isOptional;
    bool hasMinMax;
    double minValue;
    double maxValue;

    MCPParamDef()
        : type(MCPParamType::String), targetPtr(nullptr), stringCapacity(0),
          isOptional(false), hasMinMax(false), minValue(0), maxValue(0) {
        name[0] = '\0';
        description[0] = '\0';
    }
};

class MCPTool {
public:
    char name[32];
    char description[128];
    char title[64];
    const char* inputSchemaJson;
    char generatedSchema[512];
    MCPToolHandler handler;
    MCPFluentHandler fluentHandler;

    MCPParamDef params[MCP_MAX_PARAMS];
    size_t paramCount;

    bool isReadOnly;
    bool isDestructive;
    bool isIdempotent;
    uint32_t timeoutMs;

    MCPTool();
    MCPTool(const char* toolName, const char* toolDesc, MCPToolHandler toolHandler);
    MCPTool(const char* toolName, const char* toolDesc, const char* schemaJson, MCPToolHandler toolHandler);
    MCPTool(const String& toolName, const String& toolDesc, MCPToolHandler toolHandler);
    MCPTool(const String& toolName, const String& toolDesc, const String& schemaJson, MCPToolHandler toolHandler);

    bool isValid() const {
        return name[0] != '\0' && (handler != nullptr || fluentHandler != nullptr);
    }

    bool validateArgs(const MCPArguments& args, int& errorCode, char* errorMsg, size_t errorMsgCap) const;
    void buildGeneratedSchema();
};

class MCPToolRegistry;

class MCPToolBuilder {
private:
    MCPToolRegistry* _registry;
    MCPTool _tool;

public:
    MCPToolBuilder(MCPToolRegistry* registry, const char* name, const char* description);

    MCPToolBuilder& param(const char* name, bool* target, const char* desc = nullptr);
    MCPToolBuilder& param(const char* name, int* target, const char* desc = nullptr);
    MCPToolBuilder& param(const char* name, int* target, int minVal, int maxVal, const char* desc = nullptr);
    MCPToolBuilder& param(const char* name, long* target, const char* desc = nullptr);
    MCPToolBuilder& param(const char* name, long* target, long minVal, long maxVal, const char* desc = nullptr);
    MCPToolBuilder& param(const char* name, float* target, const char* desc = nullptr);
    MCPToolBuilder& param(const char* name, float* target, float minVal, float maxVal, const char* desc = nullptr);
    MCPToolBuilder& param(const char* name, double* target, const char* desc = nullptr);
    MCPToolBuilder& param(const char* name, double* target, double minVal, double maxVal, const char* desc = nullptr);
    MCPToolBuilder& param(const char* name, char* target, size_t capacity, const char* desc = nullptr);

    MCPToolBuilder& optional();
    MCPToolBuilder& title(const char* t);
    MCPToolBuilder& readOnly();
    MCPToolBuilder& destructive();
    MCPToolBuilder& idempotent();
    MCPToolBuilder& timeout(uint32_t ms);

    bool handle(MCPFluentHandler callback);
    bool handle(MCPToolHandler callback);
};

#endif
