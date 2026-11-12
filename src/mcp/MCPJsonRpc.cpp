#include "MCPJsonRpc.h"
#include "MCPServer.h"
#include <string.h>

void MCPJsonRpc::buildErrorResponse(MCPJsonWriter& writer, const MCPJsonSlice& id, int code, const char* message) {
    writer.reset();
    writer.startObject();
    writer.addString("jsonrpc", "2.0");
    if (id.isValid() && !id.isNull()) {
        writer.addRaw("id", id.ptr, id.len);
    } else {
        writer.addNull("id");
    }
    writer.writeKey("error");
    writer.startObject();
    writer.addLong("code", code);
    writer.addString("message", message);
    writer.endObject();
    writer.endObject();
}

size_t MCPJsonRpc::process(const char* request, size_t length, char* responseOut, size_t responseCapacity, MCPServer& server) {
    MCPJsonWriter writer(responseOut, responseCapacity);

    if (!request || length == 0) {
        buildErrorResponse(writer, MCPJsonSlice(), MCP_RPC_PARSE_ERROR, "Parse error");
        return writer.length();
    }

    MCPRpcRequestView reqView;
    MCPScanError scanErr = MCPScanError::None;
    if (!MCPJsonScanner::parseRpcRequest(request, length, reqView, &scanErr)) {
        buildErrorResponse(writer, MCPJsonSlice(), MCP_RPC_PARSE_ERROR, "Parse error");
        return writer.length();
    }

    if (!reqView.jsonrpc.isString() || !reqView.jsonrpc.equalsString("2.0")) {
        buildErrorResponse(writer, reqView.id, MCP_RPC_INVALID_REQUEST, "Invalid Request: jsonrpc must be '2.0'");
        return writer.length();
    }

    if (!reqView.method.isString() || reqView.method.len == 0) {
        buildErrorResponse(writer, reqView.id, MCP_RPC_INVALID_REQUEST, "Invalid Request: missing method");
        return writer.length();
    }

    if (reqView.isNotification) {
        return 0;
    }

    char method[64];
    if (!reqView.method.decodeString(method, sizeof(method))) {
        buildErrorResponse(writer, reqView.id, MCP_RPC_INVALID_REQUEST, "Invalid Request: invalid method string");
        return writer.length();
    }

    if (strcmp(method, "server/discover") == 0) {
        server.handleServerDiscover(reqView, writer);
    } else if (strcmp(method, "initialize") == 0) {
        server.handleInitialize(reqView, writer);
    } else if (strcmp(method, "ping") == 0) {
        server.handlePing(reqView, writer);
    } else if (strcmp(method, "tools/list") == 0) {
        server.handleToolsList(reqView, writer);
    } else if (strcmp(method, "tools/call") == 0) {
        server.handleToolsCall(reqView, writer);
    } else {
        char errBuf[128];
        snprintf(errBuf, sizeof(errBuf), "Method not found: %s", method);
        buildErrorResponse(writer, reqView.id, MCP_RPC_METHOD_NOT_FOUND, errBuf);
    }

    if (writer.isOverflow()) {
        buildErrorResponse(writer, reqView.id, MCP_RPC_INTERNAL_ERROR, "response too large");
    }

    return writer.length();
}

String MCPJsonRpc::process(const String& rawJson, MCPServer& server) {
    char respBuf[MCP_MAX_RESPONSE_SIZE];
    size_t len = process(rawJson.c_str(), rawJson.length(), respBuf, sizeof(respBuf), server);
    if (len == 0) {
        return String("");
    }
    return String(respBuf);
}
