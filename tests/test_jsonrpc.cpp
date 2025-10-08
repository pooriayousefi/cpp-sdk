#include <catch_amalgamated.hpp>
#include <mcp/jsonrpc/jsonrpc.hpp>

using namespace pooriayousefi::mcp::jsonrpc;
using json = nlohmann::json;

// ==================== Message Building Tests ====================

TEST_CASE("make_request creates valid requests", "[jsonrpc][request]") {
    SECTION("Request with string id and params") {
        auto req = make_request("req-1", "test_method", json{{"key", "value"}});
        
        REQUIRE(req["jsonrpc"] == "2.0");
        REQUIRE(req["method"] == "test_method");
        REQUIRE(req["id"] == "req-1");
        REQUIRE(req["params"]["key"] == "value");
    }
    
    SECTION("Request with integer id") {
        auto req = make_request(42, "test_method");
        
        REQUIRE(req["jsonrpc"] == "2.0");
        REQUIRE(req["method"] == "test_method");
        REQUIRE(req["id"] == 42);
    }
    
    SECTION("Request with unsigned integer id") {
        auto req = make_request(uint64_t(100), "test_method");
        
        REQUIRE(req["jsonrpc"] == "2.0");
        REQUIRE(req["id"] == 100);
    }
    
    SECTION("Request without params") {
        auto req = make_request("req-2", "simple_method");
        
        REQUIRE(req["jsonrpc"] == "2.0");
        REQUIRE(req["method"] == "simple_method");
        REQUIRE_FALSE(req.contains("params"));
    }
    
    SECTION("Request with array params") {
        auto req = make_request("req-3", "array_method", json::array({1, 2, 3}));
        
        REQUIRE(req["params"].is_array());
        REQUIRE(req["params"].size() == 3);
    }
}

TEST_CASE("make_notification creates valid notifications", "[jsonrpc][notification]") {
    SECTION("Notification with params") {
        auto notif = make_notification("notify_method", json{{"data", "test"}});
        
        REQUIRE(notif["jsonrpc"] == "2.0");
        REQUIRE(notif["method"] == "notify_method");
        REQUIRE_FALSE(notif.contains("id"));
        REQUIRE(notif["params"]["data"] == "test");
    }
    
    SECTION("Notification without params") {
        auto notif = make_notification("simple_notify");
        
        REQUIRE(notif["jsonrpc"] == "2.0");
        REQUIRE(notif["method"] == "simple_notify");
        REQUIRE_FALSE(notif.contains("id"));
    }
}

TEST_CASE("make_result creates valid responses", "[jsonrpc][response]") {
    SECTION("Result with string id") {
        auto resp = make_result("req-1", json{{"success", true}});
        
        REQUIRE(resp["jsonrpc"] == "2.0");
        REQUIRE(resp["id"] == "req-1");
        REQUIRE(resp["result"]["success"] == true);
        REQUIRE_FALSE(resp.contains("error"));
    }
    
    SECTION("Result with null value") {
        auto resp = make_result(42, json(nullptr));
        
        REQUIRE(resp["result"].is_null());
    }
}

TEST_CASE("make_error creates valid error responses", "[jsonrpc][error]") {
    SECTION("Standard error") {
        error e{-32601, "Method not found", nullptr};
        auto resp = make_error("req-1", e);
        
        REQUIRE(resp["jsonrpc"] == "2.0");
        REQUIRE(resp["id"] == "req-1");
        REQUIRE(resp["error"]["code"] == -32601);
        REQUIRE(resp["error"]["message"] == "Method not found");
        REQUIRE_FALSE(resp.contains("result"));
    }
    
    SECTION("Error with data") {
        error e{-32000, "Custom error", json{{"detail", "more info"}}};
        auto resp = make_error(123, e);
        
        REQUIRE(resp["error"]["code"] == -32000);
        REQUIRE(resp["error"]["message"] == "Custom error");
        REQUIRE(resp["error"]["data"]["detail"] == "more info");
    }
    
    SECTION("Predefined errors") {
        REQUIRE(parse_error.code == -32700);
        REQUIRE(invalid_request.code == -32600);
        REQUIRE(method_not_found.code == -32601);
        REQUIRE(invalid_params.code == -32602);
        REQUIRE(internal_error.code == -32603);
        REQUIRE(request_cancelled.code == -32800);
    }
}

// ==================== Validation Tests ====================

TEST_CASE("validate_request validates request messages", "[jsonrpc][validation]") {
    SECTION("Valid request") {
        json req = {{"jsonrpc", "2.0"}, {"method", "test"}, {"id", 1}};
        REQUIRE(validate_request(req));
    }
    
    SECTION("Valid notification") {
        json notif = {{"jsonrpc", "2.0"}, {"method", "test"}};
        REQUIRE(validate_request(notif));
    }
    
    SECTION("Invalid: missing method") {
        json req = {{"jsonrpc", "2.0"}, {"id", 1}};
        std::string why;
        REQUIRE_FALSE(validate_request(req, &why));
        REQUIRE(why.find("method") != std::string::npos);
    }
    
    SECTION("Invalid: wrong jsonrpc version") {
        json req = {{"jsonrpc", "1.0"}, {"method", "test"}, {"id", 1}};
        std::string why;
        REQUIRE_FALSE(validate_request(req, &why));
        REQUIRE(why.find("2.0") != std::string::npos);
    }
    
    SECTION("Invalid: method not string") {
        json req = {{"jsonrpc", "2.0"}, {"method", 123}, {"id", 1}};
        std::string why;
        REQUIRE_FALSE(validate_request(req, &why));
        REQUIRE(why.find("method") != std::string::npos);
    }
    
    SECTION("Invalid: params not array or object") {
        json req = {{"jsonrpc", "2.0"}, {"method", "test"}, {"params", "string"}, {"id", 1}};
        std::string why;
        REQUIRE_FALSE(validate_request(req, &why));
        REQUIRE(why.find("params") != std::string::npos);
    }
    
    SECTION("Invalid: bad id type") {
        json req = {{"jsonrpc", "2.0"}, {"method", "test"}, {"id", true}};
        std::string why;
        REQUIRE_FALSE(validate_request(req, &why));
        REQUIRE(why.find("id") != std::string::npos);
    }
}

TEST_CASE("validate_response validates response messages", "[jsonrpc][validation]") {
    SECTION("Valid result response") {
        json resp = {{"jsonrpc", "2.0"}, {"id", 1}, {"result", "success"}};
        REQUIRE(validate_response(resp));
    }
    
    SECTION("Valid error response") {
        json resp = {
            {"jsonrpc", "2.0"}, 
            {"id", 1}, 
            {"error", {{"code", -32601}, {"message", "Not found"}}}
        };
        REQUIRE(validate_response(resp));
    }
    
    SECTION("Invalid: missing id") {
        json resp = {{"jsonrpc", "2.0"}, {"result", "success"}};
        std::string why;
        REQUIRE_FALSE(validate_response(resp, &why));
        REQUIRE(why.find("id") != std::string::npos);
    }
    
    SECTION("Invalid: both result and error") {
        json resp = {
            {"jsonrpc", "2.0"}, 
            {"id", 1}, 
            {"result", "success"},
            {"error", {{"code", -32000}, {"message", "Error"}}}
        };
        std::string why;
        REQUIRE_FALSE(validate_response(resp, &why));
        REQUIRE(why.find("exactly one") != std::string::npos);
    }
    
    SECTION("Invalid: neither result nor error") {
        json resp = {{"jsonrpc", "2.0"}, {"id", 1}};
        std::string why;
        REQUIRE_FALSE(validate_response(resp, &why));
        REQUIRE(why.find("exactly one") != std::string::npos);
    }
    
    SECTION("Invalid: malformed error object") {
        json resp = {
            {"jsonrpc", "2.0"}, 
            {"id", 1}, 
            {"error", {{"code", "not_a_number"}}}
        };
        std::string why;
        REQUIRE_FALSE(validate_response(resp, &why));
        REQUIRE(why.find("error") != std::string::npos);
    }
}

TEST_CASE("Message type detection helpers", "[jsonrpc][helpers]") {
    SECTION("is_request detects requests") {
        json req = {{"jsonrpc", "2.0"}, {"method", "test"}, {"id", 1}};
        REQUIRE(is_request(req));
    }
    
    SECTION("is_notification detects notifications") {
        json notif = {{"jsonrpc", "2.0"}, {"method", "test"}};
        REQUIRE(is_notification(notif));
        REQUIRE(is_request(notif)); // notification is also a request
    }
    
    SECTION("is_response detects responses") {
        json resp = {{"jsonrpc", "2.0"}, {"id", 1}, {"result", "ok"}};
        REQUIRE(is_response(resp));
    }
    
    SECTION("Distinguishes request from response") {
        json req = {{"jsonrpc", "2.0"}, {"method", "test"}, {"id", 1}};
        json resp = {{"jsonrpc", "2.0"}, {"id", 1}, {"result", "ok"}};
        
        REQUIRE(is_request(req));
        REQUIRE_FALSE(is_response(req));
        
        REQUIRE(is_response(resp));
        REQUIRE_FALSE(is_request(resp));
    }
}

// ==================== Dispatcher Tests ====================

TEST_CASE("Dispatcher method registration", "[jsonrpc][dispatcher]") {
    dispatcher disp;
    
    SECTION("Register and call method") {
        bool called = false;
        disp.add("test_method", [&called](const json&) -> json {
            called = true;
            return json{{"success", true}};
        });
        
        json req = make_request("req-1", "test_method");
        auto resp = disp.handle_single(req);
        
        REQUIRE(called);
        REQUIRE(resp.has_value());
        REQUIRE((*resp)["result"]["success"] == true);
    }
    
    SECTION("Method not found") {
        json req = make_request("req-1", "unknown_method");
        auto resp = disp.handle_single(req);
        
        REQUIRE(resp.has_value());
        REQUIRE((*resp).contains("error"));
        REQUIRE((*resp)["error"]["code"] == -32601);
    }
    
    SECTION("Notification handling") {
        bool called = false;
        disp.add("notify_me", [&called](const json&) -> json {
            called = true;
            return json{};
        });
        
        json notif = make_notification("notify_me");
        auto resp = disp.handle_single(notif);
        
        REQUIRE(called);
        REQUIRE_FALSE(resp.has_value()); // No response for notifications
    }
}

TEST_CASE("Dispatcher parameter handling", "[jsonrpc][dispatcher]") {
    dispatcher disp;
    
    SECTION("Object parameters") {
        disp.add("echo_obj", [](const json& params) -> json {
            return params;
        });
        
        json req = make_request("req-1", "echo_obj", json{{"key", "value"}});
        auto resp = disp.handle_single(req);
        
        REQUIRE((*resp)["result"]["key"] == "value");
    }
    
    SECTION("Array parameters") {
        disp.add("echo_arr", [](const json& params) -> json {
            return params;
        });
        
        json req = make_request("req-1", "echo_arr", json::array({1, 2, 3}));
        auto resp = disp.handle_single(req);
        
        REQUIRE((*resp)["result"].is_array());
        REQUIRE((*resp)["result"].size() == 3);
    }
    
    SECTION("No parameters") {
        disp.add("no_params", [](const json& params) -> json {
            return json{{"received_empty", params.empty() || params.is_null()}};
        });
        
        json req = make_request("req-1", "no_params");
        auto resp = disp.handle_single(req);
        
        REQUIRE((*resp)["result"]["received_empty"] == true);
    }
}

TEST_CASE("Dispatcher error handling", "[jsonrpc][dispatcher]") {
    dispatcher disp;
    
    SECTION("Handler throws rpc_exception") {
        disp.add("error_method", [](const json&) -> json {
            throw rpc_exception(error{-32000, "Custom error", json{{"detail", "info"}}});
        });
        
        json req = make_request("req-1", "error_method");
        auto resp = disp.handle_single(req);
        
        REQUIRE((*resp).contains("error"));
        REQUIRE((*resp)["error"]["code"] == -32000);
        REQUIRE((*resp)["error"]["message"] == "Custom error");
        REQUIRE((*resp)["error"]["data"]["detail"] == "info");
    }
    
    SECTION("Handler throws standard exception") {
        disp.add("crash_method", [](const json&) -> json {
            throw std::runtime_error("Unexpected error");
        });
        
        json req = make_request("req-1", "crash_method");
        auto resp = disp.handle_single(req);
        
        REQUIRE((*resp).contains("error"));
        REQUIRE((*resp)["error"]["code"] == -32603); // Internal error
    }
    
    SECTION("Invalid request format") {
        json bad_req = {{"method", "test"}}; // Missing jsonrpc
        auto resp = disp.handle_single(bad_req);
        
        REQUIRE((*resp).contains("error"));
        REQUIRE((*resp)["error"]["code"] == -32600); // Invalid request
    }
}

// ==================== Endpoint Tests ====================

TEST_CASE("Endpoint request/response flow", "[jsonrpc][endpoint]") {
    std::vector<json> sent_messages;
    auto sender = [&sent_messages](const json& msg) {
        sent_messages.push_back(msg);
    };
    
    endpoint ep(sender);
    
    SECTION("Send request and receive response") {
        bool result_received = false;
        json result_data;
        
        ep.send_request("test_method", json{{"param", "value"}},
            [&](const json& result) {
                result_received = true;
                result_data = result;
            },
            [](const json&) {}
        );
        
        REQUIRE(sent_messages.size() == 1);
        REQUIRE(sent_messages[0]["method"] == "test_method");
        
        // Simulate response
        json req_id = sent_messages[0]["id"];
        json response = make_result(req_id, json{{"status", "ok"}});
        ep.receive(response);
        
        REQUIRE(result_received);
        REQUIRE(result_data["status"] == "ok");
    }
    
    SECTION("Send request and receive error") {
        bool error_received = false;
        json error_data;
        
        ep.send_request("error_method", json{},
            [](const json&) {},
            [&](const json& err) {
                error_received = true;
                error_data = err;
            }
        );
        
        // Simulate error response
        json req_id = sent_messages[0]["id"];
        json response = make_error(req_id, method_not_found);
        ep.receive(response);
        
        REQUIRE(error_received);
        REQUIRE(error_data["code"] == -32601);
    }
    
    SECTION("Send notification") {
        ep.send_notification("notify_event", json{{"event", "test"}});
        
        REQUIRE(sent_messages.size() == 1);
        REQUIRE(sent_messages[0]["method"] == "notify_event");
        REQUIRE_FALSE(sent_messages[0].contains("id"));
    }
}

TEST_CASE("Endpoint method handling", "[jsonrpc][endpoint]") {
    std::vector<json> sent_messages;
    endpoint ep([&sent_messages](const json& msg) {
        sent_messages.push_back(msg);
    });
    
    SECTION("Handle incoming request") {
        ep.add("test_handler", [](const json& params) -> json {
            return json{{"echo", params["input"]}};
        });
        
        json req = make_request("req-1", "test_handler", json{{"input", "hello"}});
        ep.receive(req);
        
        REQUIRE(sent_messages.size() == 1);
        REQUIRE(sent_messages[0]["result"]["echo"] == "hello");
    }
    
    SECTION("Handle incoming notification") {
        bool notif_handled = false;
        ep.add("notify_handler", [&notif_handled](const json&) -> json {
            notif_handled = true;
            return json{};
        });
        
        json notif = make_notification("notify_handler", json{{"data", "test"}});
        ep.receive(notif);
        
        REQUIRE(notif_handled);
        REQUIRE(sent_messages.empty()); // No response for notifications
    }
}

TEST_CASE("Endpoint batch processing", "[jsonrpc][endpoint]") {
    std::vector<json> sent_messages;
    endpoint ep([&sent_messages](const json& msg) {
        sent_messages.push_back(msg);
    });
    
    ep.add("method1", [](const json&) -> json { return json{{"result", 1}}; });
    ep.add("method2", [](const json&) -> json { return json{{"result", 2}}; });
    
    SECTION("Batch request processing") {
        json batch = json::array({
            make_request("req-1", "method1"),
            make_request("req-2", "method2"),
            make_notification("method1") // No response expected
        });
        
        ep.receive(batch);
        
        REQUIRE(sent_messages.size() == 1);
        REQUIRE(sent_messages[0].is_array());
        REQUIRE(sent_messages[0].size() == 2); // Only requests, not notification
    }
    
    SECTION("Empty batch is invalid") {
        json empty_batch = json::array();
        ep.receive(empty_batch);
        
        REQUIRE(sent_messages.size() == 1);
        REQUIRE(sent_messages[0].contains("error"));
        REQUIRE(sent_messages[0]["error"]["code"] == -32600);
    }
}

TEST_CASE("Endpoint cancellation support", "[jsonrpc][endpoint]") {
    std::vector<json> sent_messages;
    endpoint ep([&sent_messages](const json& msg) {
        sent_messages.push_back(msg);
    });
    
    SECTION("Cancel request handling") {
        // The endpoint automatically registers $/cancelRequest handler
        json cancel_req = make_notification("$/cancelRequest", json{{"id", "req-1"}});
        ep.receive(cancel_req);
        
        // Cancellation is processed (no error)
        // No response for notification
        REQUIRE(sent_messages.empty());
    }
}

TEST_CASE("Endpoint initialization protocol", "[jsonrpc][endpoint]") {
    std::vector<json> sent_messages;
    endpoint ep([&sent_messages](const json& msg) {
        sent_messages.push_back(msg);
    });
    
    SECTION("Send initialize request as client") {
        bool result_received = false;
        ep.initialize(
            json{{"clientInfo", {{"name", "test-client"}, {"version", "1.0"}}}},
            [&result_received](const json&) { result_received = true; },
            [](const json&) {}
        );
        
        REQUIRE(sent_messages.size() == 1);
        REQUIRE(sent_messages[0]["method"] == "initialize");
        
        // Simulate successful response
        json req_id = sent_messages[0]["id"];
        json response = make_result(req_id, json{{"capabilities", json::object()}});
        ep.receive(response);
        
        REQUIRE(result_received);
    }
    
    SECTION("Receive initialize request as server") {
        REQUIRE_FALSE(ep.is_initialized());
        
        json init_req = make_request("req-1", "initialize", json{{"clientInfo", {{"name", "client"}}}});
        ep.receive(init_req);
        
        REQUIRE(ep.is_initialized());
        REQUIRE(sent_messages.size() == 1);
        REQUIRE(sent_messages[0].contains("result"));
    }
}

// ==================== Exception Tests ====================

TEST_CASE("rpc_exception handling", "[jsonrpc][exception]") {
    SECTION("Throw and catch rpc_exception") {
        try {
            throw_rpc_error(error{-32000, "Test error", json{{"info", "test"}}});
            REQUIRE(false); // Should not reach here
        } catch (const rpc_exception& ex) {
            REQUIRE(ex.err.code == -32000);
            REQUIRE(ex.err.message == "Test error");
            REQUIRE(ex.err.data["info"] == "test");
        }
    }
}

// ==================== Edge Cases ====================

TEST_CASE("Edge cases and special scenarios", "[jsonrpc][edge_cases]") {
    SECTION("Null id in request") {
        auto req = make_request(nullptr, "test_method");
        REQUIRE_FALSE(req.contains("id"));
        REQUIRE(is_notification(req));
    }
    
    SECTION("Very large id values") {
        uint64_t large_id = 9223372036854775807ULL;
        auto req = make_request(large_id, "test_method");
        REQUIRE(req["id"] == large_id);
    }
    
    SECTION("Unicode in method names and parameters") {
        auto req = make_request("req-1", "测试方法", json{{"键", "值"}});
        REQUIRE(req["method"] == "测试方法");
        REQUIRE(req["params"]["键"] == "值");
    }
    
    SECTION("Empty method name") {
        auto req = make_request("req-1", "");
        REQUIRE(validate_request(req)); // Empty but valid
    }
    
    SECTION("Complex nested parameters") {
        json complex_params = {
            {"nested", {
                {"array", json::array({1, 2, 3})},
                {"object", {{"key", "value"}}}
            }}
        };
        auto req = make_request("req-1", "complex_method", complex_params);
        REQUIRE(req["params"]["nested"]["array"].size() == 3);
    }
}
