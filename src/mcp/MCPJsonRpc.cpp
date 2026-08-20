#include "MCPJsonRpc.h"
#include "MCPServer.h"

void MCPJsonRpc::buildErrorResponse(JsonDocument& doc, JsonVariantConst id, int code, const String& message) {
    doc.clear();
    doc["jsonrpc"] = "2.0";
    if (id.isNull()) {
        doc["id"] = nullptr;
    } else {
        doc["id"] = id;
    }
    JsonObject err = doc["error"].to<JsonObject>();
    err["code"] = code;
    err["message"] = message;
}

void MCPJsonRpc::buildSuccessResponse(JsonDocument& doc, JsonVariantConst id, JsonVariantConst result) {
    doc.clear();
    doc["jsonrpc"] = "2.0";
    if (id.isNull()) {
        doc["id"] = nullptr;
    } else {
        doc["id"] = id;
    }
    doc["result"] = result;
}

String MCPJsonRpc::process(const String& rawJson, MCPServer& server) {
    if (rawJson.length() == 0) {
        JsonDocument errDoc;
        buildErrorResponse(errDoc, JsonVariantConst(), MCP_RPC_PARSE_ERROR, "Parse error");
        String out;
        serializeJson(errDoc, out);
        return out;
    }

    JsonDocument requestDoc;
    DeserializationError err = deserializeJson(requestDoc, rawJson);
    if (err) {
        JsonDocument errDoc;
        buildErrorResponse(errDoc, JsonVariantConst(), MCP_RPC_PARSE_ERROR, "Parse error");
        String out;
        serializeJson(errDoc, out);
        return out;
    }

    if (!requestDoc.is<JsonObject>()) {
        JsonDocument errDoc;
        buildErrorResponse(errDoc, JsonVariantConst(), MCP_RPC_INVALID_REQUEST, "Invalid Request");
        String out;
        serializeJson(errDoc, out);
        return out;
    }

    JsonObjectConst req = requestDoc.as<JsonObjectConst>();

    if (req["jsonrpc"].isNull() || String(req["jsonrpc"].as<const char*>()) != "2.0") {
        JsonDocument errDoc;
        JsonVariantConst id = req["id"];
        buildErrorResponse(errDoc, id, MCP_RPC_INVALID_REQUEST, "Invalid Request: jsonrpc must be '2.0'");
        String out;
        serializeJson(errDoc, out);
        return out;
    }

    if (req["method"].isNull() || !req["method"].is<const char*>()) {
        JsonDocument errDoc;
        JsonVariantConst id = req["id"];
        buildErrorResponse(errDoc, id, MCP_RPC_INVALID_REQUEST, "Invalid Request: missing method");
        String out;
        serializeJson(errDoc, out);
        return out;
    }

    String method = req["method"].as<const char*>();
    bool isNotification = req["id"].isNull();
    JsonVariantConst id = req["id"];
    JsonObjectConst params = req["params"].as<JsonObjectConst>();

    if (isNotification) {
        return "";
    }

    JsonDocument responseDoc;
    JsonObject resultObj = responseDoc["result"].to<JsonObject>();

    if (method == "server/discover") {
        if (server.handleServerDiscover(params, resultObj)) {
            responseDoc["jsonrpc"] = "2.0";
            responseDoc["id"] = id;
        } else {
            buildErrorResponse(responseDoc, id, MCP_RPC_INTERNAL_ERROR, "Discovery failed");
        }
    } else if (method == "initialize") {
        if (server.handleInitialize(params, resultObj)) {
            responseDoc["jsonrpc"] = "2.0";
            responseDoc["id"] = id;
        } else {
            buildErrorResponse(responseDoc, id, MCP_RPC_INTERNAL_ERROR, "Initialize failed");
        }
    } else if (method == "ping") {
        server.handlePing(params, resultObj);
        responseDoc["jsonrpc"] = "2.0";
        responseDoc["id"] = id;
    } else if (method == "tools/list") {
        if (server.handleToolsList(params, resultObj)) {
            responseDoc["jsonrpc"] = "2.0";
            responseDoc["id"] = id;
        } else {
            buildErrorResponse(responseDoc, id, MCP_RPC_INTERNAL_ERROR, "Failed to list tools");
        }
    } else if (method == "tools/call") {
        bool isProtocolError = false;
        int errorCode = 0;
        String errorMessage = "";
        bool ok = server.handleToolsCall(params, resultObj, isProtocolError, errorCode, errorMessage);
        if (!ok && isProtocolError) {
            buildErrorResponse(responseDoc, id, errorCode, errorMessage);
        } else {
            responseDoc["jsonrpc"] = "2.0";
            responseDoc["id"] = id;
        }
    } else {
        buildErrorResponse(responseDoc, id, MCP_RPC_METHOD_NOT_FOUND, "Method not found: " + method);
    }

    String output;
    serializeJson(responseDoc, output);
    return output;
}
