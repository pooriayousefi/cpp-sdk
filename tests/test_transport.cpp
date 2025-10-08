#include <catch_amalgamated.hpp>
#include <mcp/transport/transport.hpp>
#include <thread>
#include <chrono>
#include <atomic>

using namespace pooriayousefi::mcp::transport;
using json = nlohmann::json;

// Helper to wait for async events with timeout
template<typename Predicate>
bool wait_for_condition(Predicate pred, int timeout_ms = 1000) {
    auto start = std::chrono::steady_clock::now();
    while (!pred()) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count() > timeout_ms) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

// ==================== InMemoryTransport Tests ====================

TEST_CASE("InMemoryTransport basic functionality", "[transport][inmemory]") {
    auto [client, server] = create_in_memory_pair();
    
    SECTION("Initial state") {
        REQUIRE_FALSE(client->is_open());
        REQUIRE_FALSE(server->is_open());
    }
    
    SECTION("Start and stop") {
        client->start();
        server->start();
        
        REQUIRE(client->is_open());
        REQUIRE(server->is_open());
        
        client->close();
        server->close();
        
        REQUIRE_FALSE(client->is_open());
        REQUIRE_FALSE(server->is_open());
    }
}

TEST_CASE("InMemoryTransport message passing", "[transport][inmemory]") {
    auto [client, server] = create_in_memory_pair();
    
    SECTION("Client to server message") {
        std::atomic<bool> message_received{false};
        json received_message;
        
        server->on_message([&](const json& msg) {
            received_message = msg;
            message_received = true;
        });
        
        client->start();
        server->start();
        
        json test_msg = {{"method", "test"}, {"id", 1}};
        client->send(test_msg);
        
        REQUIRE(wait_for_condition([&]() { return message_received.load(); }));
        REQUIRE(received_message["method"] == "test");
        REQUIRE(received_message["id"] == 1);
        
        client->close();
        server->close();
    }
    
    SECTION("Server to client message") {
        std::atomic<bool> message_received{false};
        json received_message;
        
        client->on_message([&](const json& msg) {
            received_message = msg;
            message_received = true;
        });
        
        client->start();
        server->start();
        
        json response = {{"result", "success"}, {"id", 1}};
        server->send(response);
        
        REQUIRE(wait_for_condition([&]() { return message_received.load(); }));
        REQUIRE(received_message["result"] == "success");
        
        client->close();
        server->close();
    }
    
    SECTION("Bidirectional communication") {
        std::atomic<int> client_count{0};
        std::atomic<int> server_count{0};
        
        client->on_message([&](const json&) { client_count++; });
        server->on_message([&](const json&) { server_count++; });
        
        client->start();
        server->start();
        
        // Send multiple messages both ways
        for (int i = 0; i < 5; i++) {
            client->send(json{{"from", "client"}, {"num", i}});
            server->send(json{{"from", "server"}, {"num", i}});
        }
        
        REQUIRE(wait_for_condition([&]() { 
            return client_count.load() == 5 && server_count.load() == 5; 
        }));
        
        client->close();
        server->close();
    }
}

TEST_CASE("InMemoryTransport error handling", "[transport][inmemory]") {
    auto [client, server] = create_in_memory_pair();
    
    SECTION("Send before start") {
        std::atomic<bool> error_received{false};
        std::string error_msg;
        
        client->on_error([&](const std::string& err) {
            error_msg = err;
            error_received = true;
        });
        
        client->send(json{{"test", "message"}});
        
        REQUIRE(wait_for_condition([&]() { return error_received.load(); }, 500));
        REQUIRE(error_msg.find("not started") != std::string::npos);
    }
    
    SECTION("Close handler") {
        std::atomic<bool> close_called{false};
        
        client->on_close([&]() {
            close_called = true;
        });
        
        client->start();
        client->close();
        
        REQUIRE(wait_for_condition([&]() { return close_called.load(); }, 500));
    }
}

TEST_CASE("InMemoryTransport complex message types", "[transport][inmemory]") {
    auto [client, server] = create_in_memory_pair();
    
    SECTION("Large JSON message") {
        std::atomic<bool> received{false};
        json received_msg;
        
        server->on_message([&](const json& msg) {
            received_msg = msg;
            received = true;
        });
        
        client->start();
        server->start();
        
        // Create large nested JSON
        json large_msg = {
            {"type", "test"},
            {"data", json::array()},
            {"nested", {
                {"level1", {
                    {"level2", {
                        {"values", json::array()}
                    }}
                }}
            }}
        };
        
        for (int i = 0; i < 100; i++) {
            large_msg["data"].push_back({{"id", i}, {"value", std::to_string(i)}});
            large_msg["nested"]["level1"]["level2"]["values"].push_back(i);
        }
        
        client->send(large_msg);
        
        REQUIRE(wait_for_condition([&]() { return received.load(); }));
        REQUIRE(received_msg["data"].size() == 100);
        REQUIRE(received_msg["nested"]["level1"]["level2"]["values"].size() == 100);
        
        client->close();
        server->close();
    }
    
    SECTION("Unicode messages") {
        std::atomic<bool> received{false};
        json received_msg;
        
        server->on_message([&](const json& msg) {
            received_msg = msg;
            received = true;
        });
        
        client->start();
        server->start();
        
        json unicode_msg = {
            {"chinese", "你好世界"},
            {"japanese", "こんにちは"},
            {"emoji", "🚀🎉✨"},
            {"mixed", "Hello 世界 🌍"}
        };
        
        client->send(unicode_msg);
        
        REQUIRE(wait_for_condition([&]() { return received.load(); }));
        REQUIRE(received_msg["chinese"] == "你好世界");
        REQUIRE(received_msg["emoji"] == "🚀🎉✨");
        
        client->close();
        server->close();
    }
}

TEST_CASE("InMemoryTransport multiple rapid messages", "[transport][inmemory]") {
    auto [client, server] = create_in_memory_pair();
    
    SECTION("Rapid fire messages") {
        std::atomic<int> count{0};
        std::vector<int> received_ids;
        std::mutex received_mutex;
        
        server->on_message([&](const json& msg) {
            std::lock_guard<std::mutex> lock(received_mutex);
            received_ids.push_back(msg["id"].get<int>());
            count++;
        });
        
        client->start();
        server->start();
        
        const int num_messages = 100;
        for (int i = 0; i < num_messages; i++) {
            client->send(json{{"id", i}});
        }
        
        REQUIRE(wait_for_condition([&]() { return count.load() == num_messages; }, 2000));
        
        // Verify all messages received (order may vary)
        std::lock_guard<std::mutex> lock(received_mutex);
        REQUIRE(received_ids.size() == num_messages);
        
        client->close();
        server->close();
    }
}

TEST_CASE("create_in_memory_pair creates connected transports", "[transport][inmemory]") {
    SECTION("Pair creation") {
        auto [t1, t2] = create_in_memory_pair();
        
        REQUIRE(t1 != nullptr);
        REQUIRE(t2 != nullptr);
        REQUIRE(t1 != t2);
    }
    
    SECTION("Pair is pre-connected") {
        auto [t1, t2] = create_in_memory_pair();
        
        std::atomic<bool> t2_received{false};
        t2->on_message([&](const json&) { t2_received = true; });
        
        t1->start();
        t2->start();
        
        t1->send(json{{"test", true}});
        
        REQUIRE(wait_for_condition([&]() { return t2_received.load(); }));
        
        t1->close();
        t2->close();
    }
}

// ==================== Transport Handler Tests ====================

TEST_CASE("Transport handler registration", "[transport][handlers]") {
    auto [client, server] = create_in_memory_pair();
    
    SECTION("Message handler") {
        bool handler_called = false;
        
        client->on_message([&](const json&) {
            handler_called = true;
        });
        
        client->start();
        server->start();
        
        server->send(json{{"test", 1}});
        
        REQUIRE(wait_for_condition([&]() { return handler_called; }));
        
        client->close();
        server->close();
    }
    
    SECTION("Error handler") {
        std::string error_message;
        
        client->on_error([&](const std::string& err) {
            error_message = err;
        });
        
        // Trigger error by sending without starting
        client->send(json{{"test", 1}});
        
        REQUIRE(wait_for_condition([&]() { return !error_message.empty(); }, 500));
        REQUIRE_FALSE(error_message.empty());
    }
    
    SECTION("Close handler") {
        bool close_called = false;
        
        client->on_close([&]() {
            close_called = true;
        });
        
        client->start();
        client->close();
        
        REQUIRE(wait_for_condition([&]() { return close_called; }, 500));
    }
    
    SECTION("Multiple handlers can be set") {
        int message_count = 0;
        int error_count = 0;
        int close_count = 0;
        
        client->on_message([&](const json&) { message_count++; });
        client->on_error([&](const std::string&) { error_count++; });
        client->on_close([&]() { close_count++; });
        
        client->start();
        server->start();
        
        server->send(json{{"msg", 1}});
        server->send(json{{"msg", 2}});
        
        REQUIRE(wait_for_condition([&]() { return message_count == 2; }));
        
        client->close();
        server->close();
        
        REQUIRE(wait_for_condition([&]() { return close_count == 1; }, 500));
    }
}

// ==================== Transport Lifecycle Tests ====================

TEST_CASE("Transport lifecycle management", "[transport][lifecycle]") {
    SECTION("Start twice is idempotent") {
        auto [client, server] = create_in_memory_pair();
        
        client->start();
        client->start(); // Second start should be no-op
        
        REQUIRE(client->is_open());
        
        client->close();
    }
    
    SECTION("Close twice is safe") {
        auto [client, server] = create_in_memory_pair();
        
        client->start();
        client->close();
        client->close(); // Second close should be safe
        
        REQUIRE_FALSE(client->is_open());
    }
    
    SECTION("Restart transport") {
        auto [client, server] = create_in_memory_pair();
        
        std::atomic<int> message_count{0};
        server->on_message([&](const json&) { message_count++; });
        
        // First session
        client->start();
        server->start();
        client->send(json{{"session", 1}});
        REQUIRE(wait_for_condition([&]() { return message_count.load() == 1; }));
        client->close();
        server->close();
        
        // Restart
        client->start();
        server->start();
        client->send(json{{"session", 2}});
        REQUIRE(wait_for_condition([&]() { return message_count.load() == 2; }));
        client->close();
        server->close();
        
        REQUIRE(message_count.load() == 2);
    }
}

// ==================== Integration Scenarios ====================

TEST_CASE("Transport integration scenarios", "[transport][integration]") {
    SECTION("Request-response pattern") {
        auto [client, server] = create_in_memory_pair();
        
        std::atomic<bool> request_received{false};
        std::atomic<bool> response_received{false};
        json client_response;
        
        // Server echoes request
        server->on_message([&](const json& req) {
            request_received = true;
            json resp = {
                {"jsonrpc", "2.0"},
                {"id", req["id"]},
                {"result", req["params"]}
            };
            server->send(resp);
        });
        
        // Client receives response
        client->on_message([&](const json& resp) {
            client_response = resp;
            response_received = true;
        });
        
        client->start();
        server->start();
        
        json request = {
            {"jsonrpc", "2.0"},
            {"method", "echo"},
            {"params", {{"msg", "hello"}}},
            {"id", 1}
        };
        client->send(request);
        
        REQUIRE(wait_for_condition([&]() { 
            return request_received.load() && response_received.load(); 
        }));
        REQUIRE(client_response["result"]["msg"] == "hello");
        
        client->close();
        server->close();
    }
    
    SECTION("Notification pattern (no response)") {
        auto [client, server] = create_in_memory_pair();
        
        std::atomic<int> notification_count{0};
        
        server->on_message([&](const json& msg) {
            if (!msg.contains("id")) { // Notification
                notification_count++;
            }
        });
        
        client->start();
        server->start();
        
        // Send notifications
        client->send(json{{"jsonrpc", "2.0"}, {"method", "notify1"}});
        client->send(json{{"jsonrpc", "2.0"}, {"method", "notify2"}});
        client->send(json{{"jsonrpc", "2.0"}, {"method", "notify3"}});
        
        REQUIRE(wait_for_condition([&]() { return notification_count.load() == 3; }));
        
        client->close();
        server->close();
    }
    
    SECTION("Concurrent bidirectional communication") {
        auto [client, server] = create_in_memory_pair();
        
        std::atomic<int> client_received{0};
        std::atomic<int> server_received{0};
        
        client->on_message([&](const json&) { client_received++; });
        server->on_message([&](const json&) { server_received++; });
        
        client->start();
        server->start();
        
        // Both sides send simultaneously
        std::thread client_thread([&]() {
            for (int i = 0; i < 10; i++) {
                client->send(json{{"from", "client"}, {"i", i}});
            }
        });
        
        std::thread server_thread([&]() {
            for (int i = 0; i < 10; i++) {
                server->send(json{{"from", "server"}, {"i", i}});
            }
        });
        
        client_thread.join();
        server_thread.join();
        
        REQUIRE(wait_for_condition([&]() { 
            return client_received.load() == 10 && server_received.load() == 10; 
        }));
        
        client->close();
        server->close();
    }
}

// ==================== Edge Cases ====================

TEST_CASE("Transport edge cases", "[transport][edge_cases]") {
    SECTION("Empty JSON object") {
        auto [client, server] = create_in_memory_pair();
        
        std::atomic<bool> received{false};
        json received_msg;
        
        server->on_message([&](const json& msg) {
            received_msg = msg;
            received = true;
        });
        
        client->start();
        server->start();
        
        client->send(json::object());
        
        REQUIRE(wait_for_condition([&]() { return received.load(); }));
        REQUIRE(received_msg.is_object());
        REQUIRE(received_msg.empty());
        
        client->close();
        server->close();
    }
    
    SECTION("Null values in JSON") {
        auto [client, server] = create_in_memory_pair();
        
        std::atomic<bool> received{false};
        json received_msg;
        
        server->on_message([&](const json& msg) {
            received_msg = msg;
            received = true;
        });
        
        client->start();
        server->start();
        
        client->send(json{{"key", nullptr}, {"id", json(nullptr)}});
        
        REQUIRE(wait_for_condition([&]() { return received.load(); }));
        REQUIRE(received_msg["key"].is_null());
        
        client->close();
        server->close();
    }
    
    SECTION("Very long string values") {
        auto [client, server] = create_in_memory_pair();
        
        std::atomic<bool> received{false};
        json received_msg;
        
        server->on_message([&](const json& msg) {
            received_msg = msg;
            received = true;
        });
        
        client->start();
        server->start();
        
        std::string long_string(10000, 'x');
        client->send(json{{"data", long_string}});
        
        REQUIRE(wait_for_condition([&]() { return received.load(); }));
        REQUIRE(received_msg["data"].get<std::string>().size() == 10000);
        
        client->close();
        server->close();
    }
}
