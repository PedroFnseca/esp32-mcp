#include "MCPToolRegistry.h"
#include "json/MCPJsonScanner.h"
#include <string.h>

MCPToolRegistry::MCPToolRegistry() : _count(0) {
}

MCPToolRegistry::~MCPToolRegistry() {
    clear();
}

bool MCPToolRegistry::registerTool(const MCPTool& tool) {
    if (!tool.isValid()) {
        return false;
    }

    if (tool.inputSchemaJson && tool.inputSchemaJson[0] != '\0') {
        const char* s = tool.inputSchemaJson;
        size_t len = strlen(s);
        MCPScanError err = MCPScanError::None;
        if (!MCPJsonScanner::validate(s, len, &err)) {
            return false;
        }
        const char* p = MCPJsonScanner::skipWhitespace(s, s + len);
        if (*p != '{') {
            return false;
        }
    }

    for (size_t i = 0; i < _count; ++i) {
        if (strcmp(_tools[i].name, tool.name) == 0) {
            _tools[i] = tool;
            return true;
        }
    }

    if (_count >= MCP_MAX_TOOLS) {
        return false;
    }

    _tools[_count++] = tool;
    return true;
}

bool MCPToolRegistry::registerTool(const char* name, const char* description, MCPToolHandler handler) {
    return registerTool(MCPTool(name, description, handler));
}

bool MCPToolRegistry::registerTool(const char* name, const char* description, const char* schemaJson, MCPToolHandler handler) {
    return registerTool(MCPTool(name, description, schemaJson, handler));
}

bool MCPToolRegistry::registerTool(const String& name, const String& description, MCPToolHandler handler) {
    return registerTool(name.c_str(), description.c_str(), handler);
}

bool MCPToolRegistry::registerTool(const String& name, const String& description, const String& schemaJson, MCPToolHandler handler) {
    return registerTool(name.c_str(), description.c_str(), schemaJson.c_str(), handler);
}

const MCPTool* MCPToolRegistry::findTool(const char* name) const {
    if (!name) return nullptr;
    for (size_t i = 0; i < _count; ++i) {
        if (strcmp(_tools[i].name, name) == 0) {
            return &_tools[i];
        }
    }
    return nullptr;
}

const MCPTool* MCPToolRegistry::findTool(const String& name) const {
    return findTool(name.c_str());
}

const MCPTool* MCPToolRegistry::getTool(size_t index) const {
    if (index >= _count) {
        return nullptr;
    }
    return &_tools[index];
}

size_t MCPToolRegistry::count() const {
    return _count;
}

void MCPToolRegistry::clear() {
    for (size_t i = 0; i < _count; ++i) {
        _tools[i] = MCPTool();
    }
    _count = 0;
}
