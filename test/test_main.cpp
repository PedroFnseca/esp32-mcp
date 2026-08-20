#include <iostream>
#include <cassert>
#include "Arduino.h"
#include "ArduinoJson.h"
#include "../src/mcp/MCPTypes.h"
#include "../src/mcp/MCPTool.h"
#include "../src/mcp/MCPToolRegistry.h"
#include "../src/mcp/MCPJsonRpc.h"
#include "../src/mcp/MCPServer.h"

int tests_passed = 0;
int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define RUN_TEST(fn) \
    do { \
        int before = tests_failed; \
        std::cout << "[RUNNING] " << #fn << "..."; \
        fn(); \
        if (tests_failed == before) { \
            tests_passed++; \
            std::cout << " [PASS]" << std::endl; \
        } \
    } while(0)

void test_json_rpc_parse_error() {
    MCPServer server;
    String response = server.processMessage("invalid json {");
    JsonDocument doc;
    deserializeJson(doc, response);
    TEST_ASSERT(doc["jsonrpc"] == "2.0", "jsonrpc must be 2.0");
    TEST_ASSERT(doc["error"]["code"] == -32700, "Error code must be -32700");
}

void test_json_rpc_empty_message() {
    MCPServer server;
    String response = server.processMessage("");
    JsonDocument doc;
    deserializeJson(doc, response);
    TEST_ASSERT(doc["jsonrpc"] == "2.0", "jsonrpc must be 2.0");
    TEST_ASSERT(doc["error"]["code"] == -32700, "Error code must be -32700");
}

void test_json_rpc_invalid_request() {
    MCPServer server;
    String response = server.processMessage("{\"hello\":\"world\"}");
    JsonDocument doc;
    deserializeJson(doc, response);
    TEST_ASSERT(doc["error"]["code"] == -32600, "Error code must be -32600");
}

void test_json_rpc_unknown_method() {
    MCPServer server;
    String req = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"unknown/method\",\"params\":{}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);
    TEST_ASSERT(doc["id"] == 1, "ID must match");
    TEST_ASSERT(doc["error"]["code"] == -32601, "Error code must be -32601");
}

void test_mcp_server_discover() {
    MCPServer server("ESP32-DiscoverNode", "2.0.0");
    server.setInstructions("Autonomous IoT Node");
    String req = "{\"jsonrpc\":\"2.0\",\"id\":\"disc-1\",\"method\":\"server/discover\",\"params\":{}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == "disc-1", "ID must match");
    TEST_ASSERT(doc["result"]["supportedVersions"][0] == "2026-07-28", "Supported version must be 2026-07-28");
    TEST_ASSERT(doc["result"]["capabilities"]["tools"]["listChanged"] == false, "Capabilities tools mismatch");
    TEST_ASSERT(doc["result"]["instructions"] == "Autonomous IoT Node", "Instructions mismatch");
    TEST_ASSERT(doc["result"]["_meta"]["io.modelcontextprotocol/serverInfo"]["name"] == "ESP32-DiscoverNode", "Meta server name mismatch");
    TEST_ASSERT(doc["result"]["serverInfo"]["name"] == "ESP32-DiscoverNode", "ServerInfo name mismatch");
}

void test_mcp_initialize_2026() {
    MCPServer server("TestESP32", "1.2.3");
    server.setInstructions("Test instructions for AI");
    String req = "{\"jsonrpc\":\"2.0\",\"id\":42,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2026-07-28\",\"capabilities\":{},\"clientInfo\":{\"name\":\"TestClient\",\"version\":\"1.0\"}}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == 42, "ID must be 42");
    TEST_ASSERT(doc["result"]["protocolVersion"] == "2026-07-28", "Protocol version must be 2026-07-28");
    TEST_ASSERT(doc["result"]["serverInfo"]["name"] == "TestESP32", "Server name mismatch");
    TEST_ASSERT(doc["result"]["serverInfo"]["version"] == "1.2.3", "Server version mismatch");
    TEST_ASSERT(doc["result"]["instructions"] == "Test instructions for AI", "Instructions mismatch");
    TEST_ASSERT(doc["result"]["capabilities"]["tools"]["listChanged"] == false, "Capabilities tools mismatch");
}

void test_mcp_ping() {
    MCPServer server;
    String req = "{\"jsonrpc\":\"2.0\",\"id\":\"ping-123\",\"method\":\"ping\"}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == "ping-123", "ID must match");
    TEST_ASSERT(!doc["result"].isNull(), "Must contain result object");
}

void test_mcp_notification_no_response() {
    MCPServer server;
    String req = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}";
    String response = server.processMessage(req);
    TEST_ASSERT(response.length() == 0, "Notifications must return empty response");
}

void test_tools_list() {
    MCPServer server;
    server.tool("get_temp", "Reads temperature", [](const MCPArguments& args) {
        return MCPResult::text("25.5 C");
    });
    server.tool(
        "set_led",
        "Control LED",
        "{\"type\":\"object\",\"properties\":{\"state\":{\"type\":\"boolean\"}},\"required\":[\"state\"]}",
        [](const MCPArguments& args) {
            return MCPResult::text("OK");
        }
    );

    String req = "{\"jsonrpc\":\"2.0\",\"id\":\"req-list\",\"method\":\"tools/list\"}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == "req-list", "ID must match");
    JsonArray tools = doc["result"]["tools"].as<JsonArray>();
    TEST_ASSERT(tools.size() == 2, "Must have 2 tools");
    TEST_ASSERT(tools[0]["name"] == "get_temp", "First tool name mismatch");
    TEST_ASSERT(tools[0]["inputSchema"]["type"] == "object", "Default schema type must be object");
    TEST_ASSERT(tools[1]["name"] == "set_led", "Second tool name mismatch");
    TEST_ASSERT(tools[1]["inputSchema"]["type"] == "object", "Schema type must be object");
    TEST_ASSERT(tools[1]["inputSchema"]["properties"]["state"]["type"] == "boolean", "Schema property type mismatch");
}

void test_tools_call_success() {
    MCPServer server;
    server.tool("add_numbers", "Adds two numbers", [](const MCPArguments& args) {
        int a = args.getInt("a");
        int b = args.getInt("b");
        int sum = a + b;
        return MCPResult::text(String(sum));
    });

    String req = "{\"jsonrpc\":\"2.0\",\"id\":10,\"method\":\"tools/call\",\"params\":{\"name\":\"add_numbers\",\"arguments\":{\"a\":15,\"b\":27}}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == 10, "ID must be 10");
    TEST_ASSERT(doc["result"]["isError"] == false, "isError must be false");
    JsonArray content = doc["result"]["content"].as<JsonArray>();
    TEST_ASSERT(content.size() == 1, "Content array size must be 1");
    TEST_ASSERT(content[0]["type"] == "text", "Content type must be text");
    TEST_ASSERT(content[0]["text"] == "42", "Addition result must be 42");
}

void test_tools_call_error_handling() {
    MCPServer server;
    server.tool("failing_tool", "Tool that fails", [](const MCPArguments& args) {
        return MCPResult::error("DEVICE_BUSY", "Sensor currently busy");
    });

    String req = "{\"jsonrpc\":\"2.0\",\"id\":11,\"method\":\"tools/call\",\"params\":{\"name\":\"failing_tool\",\"arguments\":{}}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == 11, "ID must be 11");
    TEST_ASSERT(doc["result"]["isError"] == true, "isError must be true");
    TEST_ASSERT(doc["result"]["errorCode"] == "DEVICE_BUSY", "Error code mismatch");
    TEST_ASSERT(doc["result"]["content"][0]["text"] == "Sensor currently busy", "Error message mismatch");
}

void test_tools_call_unknown_tool() {
    MCPServer server;
    String req = "{\"jsonrpc\":\"2.0\",\"id\":12,\"method\":\"tools/call\",\"params\":{\"name\":\"non_existent\",\"arguments\":{}}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);

    TEST_ASSERT(doc["id"] == 12, "ID must be 12");
    TEST_ASSERT(doc["error"]["code"] == -32602, "Error code must be -32602 for unknown tool");
}

void test_tools_overwrite_registration() {
    MCPServer server;
    server.tool("sensor", "V1 description", [](const MCPArguments& args) {
        return MCPResult::text("V1");
    });
    server.tool("sensor", "V2 description", [](const MCPArguments& args) {
        return MCPResult::text("V2");
    });

    TEST_ASSERT(server.getToolRegistry().count() == 1, "Must have only 1 tool after overwrite");
    const MCPTool* t = server.getToolRegistry().findTool("sensor");
    TEST_ASSERT(t != nullptr, "Tool must exist");
    TEST_ASSERT(t->description == "V2 description", "Description must be updated");

    String req = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\"sensor\",\"arguments\":{}}}";
    String response = server.processMessage(req);
    JsonDocument doc;
    deserializeJson(doc, response);
    TEST_ASSERT(doc["result"]["content"][0]["text"] == "V2", "Result must come from updated handler");
}

void test_mcp_arguments_types() {
    JsonDocument doc;
    doc["str_key"] = "hello";
    doc["int_key"] = 123;
    doc["float_key"] = 45.67;
    doc["bool_key"] = true;

    MCPArguments args(doc.as<JsonObjectConst>());
    TEST_ASSERT(args.has("str_key"), "Must have str_key");
    TEST_ASSERT(!args.has("missing_key"), "Must not have missing_key");
    TEST_ASSERT(args.getString("str_key") == "hello", "String value mismatch");
    TEST_ASSERT(args.getInt("int_key") == 123, "Int value mismatch");
    TEST_ASSERT(args.getFloat("float_key") > 45.0f, "Float value mismatch");
    TEST_ASSERT(args.getBool("bool_key") == true, "Bool value mismatch");
    TEST_ASSERT(args.getInt("missing_key", 999) == 999, "Default value mismatch");
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " Running ESP32-MCP Unit Test Suite      " << std::endl;
    std::cout << "========================================" << std::endl;

    RUN_TEST(test_json_rpc_parse_error);
    RUN_TEST(test_json_rpc_empty_message);
    RUN_TEST(test_json_rpc_invalid_request);
    RUN_TEST(test_json_rpc_unknown_method);
    RUN_TEST(test_mcp_server_discover);
    RUN_TEST(test_mcp_initialize_2026);
    RUN_TEST(test_mcp_ping);
    RUN_TEST(test_mcp_notification_no_response);
    RUN_TEST(test_tools_list);
    RUN_TEST(test_tools_call_success);
    RUN_TEST(test_tools_call_error_handling);
    RUN_TEST(test_tools_call_unknown_tool);
    RUN_TEST(test_tools_overwrite_registration);
    RUN_TEST(test_mcp_arguments_types);

    std::cout << "========================================" << std::endl;
    std::cout << " Test Summary: " << tests_passed << " passed, " << tests_failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return tests_failed == 0 ? 0 : 1;
}
