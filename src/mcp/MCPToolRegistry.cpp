#include "MCPToolRegistry.h"

MCPToolRegistry::MCPToolRegistry() {
}

MCPToolRegistry::~MCPToolRegistry() {
    _tools.clear();
}

bool MCPToolRegistry::registerTool(const MCPTool& tool) {
    if (!tool.isValid()) {
        return false;
    }
    for (size_t i = 0; i < _tools.size(); ++i) {
        if (_tools[i].name == tool.name) {
            _tools[i] = tool;
            return true;
        }
    }
    _tools.push_back(tool);
    return true;
}

bool MCPToolRegistry::registerTool(const String& name, const String& description, MCPToolHandler handler) {
    return registerTool(MCPTool(name, description, handler));
}

bool MCPToolRegistry::registerTool(const String& name, const String& description, const String& schemaJson, MCPToolHandler handler) {
    return registerTool(MCPTool(name, description, schemaJson, handler));
}

const MCPTool* MCPToolRegistry::findTool(const String& name) const {
    for (size_t i = 0; i < _tools.size(); ++i) {
        if (_tools[i].name == name) {
            return &_tools[i];
        }
    }
    return nullptr;
}

const std::vector<MCPTool>& MCPToolRegistry::getTools() const {
    return _tools;
}

size_t MCPToolRegistry::count() const {
    return _tools.size();
}

void MCPToolRegistry::clear() {
    _tools.clear();
}
