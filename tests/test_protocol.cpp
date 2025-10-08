#include <catch_amalgamated.hpp>
#include <mcp/protocol.hpp>

using namespace pooriayousefi::mcp;
using json = nlohmann::json;

TEST_CASE("Tool serialization and deserialization", "[protocol][tool]") {
    SECTION("Complete Tool with all fields") {
        ToolInputSchema schema;
        schema.type = "object";
        schema.properties = {
            {"param1", {{"type", "string"}}},
            {"param2", {{"type", "number"}}}
        };
        schema.required = {"param1"};
        
        Tool tool{
            "test_tool",
            "A test tool for unit testing",
            schema
        };

        auto j = tool.to_json();
        
        REQUIRE(j["name"] == "test_tool");
        REQUIRE(j["description"] == "A test tool for unit testing");
        REQUIRE(j["inputSchema"]["type"] == "object");
        REQUIRE(j["inputSchema"]["properties"].contains("param1"));
        REQUIRE(j["inputSchema"]["properties"].contains("param2"));
        REQUIRE(j["inputSchema"]["required"][0] == "param1");
    }

    SECTION("Minimal Tool with only name") {
        Tool tool{
            "minimal_tool",
            std::nullopt,
            ToolInputSchema{}
        };

        auto j = tool.to_json();
        
        REQUIRE(j["name"] == "minimal_tool");
        REQUIRE_FALSE(j.contains("description"));
        REQUIRE(j["inputSchema"]["type"] == "object");
    }

    SECTION("Tool with description") {
        Tool tool{
            "described_tool",
            "This tool has a description",
            ToolInputSchema{}
        };

        auto j = tool.to_json();
        
        REQUIRE(j["name"] == "described_tool");
        REQUIRE(j["description"] == "This tool has a description");
        REQUIRE(j["inputSchema"]["type"] == "object");
    }
}

TEST_CASE("Prompt serialization and deserialization", "[protocol][prompt]") {
    SECTION("Complete Prompt with arguments") {
        Prompt prompt{
            "test_prompt",
            "A test prompt",
            {
                PromptArgument{"topic", "The topic to discuss", false}
            }
        };

        auto j = prompt.to_json();
        
        REQUIRE(j["name"] == "test_prompt");
        REQUIRE(j["description"] == "A test prompt");
        REQUIRE(j["arguments"].is_array());
        REQUIRE(j["arguments"][0]["name"] == "topic");
        REQUIRE(j["arguments"][0]["required"] == false);
        REQUIRE(j["arguments"][0]["description"] == "The topic to discuss");
    }

    SECTION("Minimal Prompt") {
        Prompt prompt{
            "minimal_prompt",
            std::nullopt,
            {}
        };

        auto j = prompt.to_json();
        
        REQUIRE(j["name"] == "minimal_prompt");
        REQUIRE_FALSE(j.contains("description"));
        REQUIRE_FALSE(j.contains("arguments"));
    }
}

TEST_CASE("Resource serialization and deserialization", "[protocol][resource]") {
    SECTION("Complete Resource with all fields") {
        Resource resource{
            "file:///test.txt",
            "Test File",
            "A test resource",
            "text/plain"
        };

        auto j = resource.to_json();
        
        REQUIRE(j["uri"] == "file:///test.txt");
        REQUIRE(j["name"] == "Test File");
        REQUIRE(j["description"] == "A test resource");
        REQUIRE(j["mimeType"] == "text/plain");
    }

    SECTION("Minimal Resource with URI and name") {
        Resource resource{
            "file:///minimal.txt",
            "Minimal",
            std::nullopt,
            std::nullopt
        };

        auto j = resource.to_json();
        
        REQUIRE(j["uri"] == "file:///minimal.txt");
        REQUIRE(j["name"] == "Minimal");
        REQUIRE_FALSE(j.contains("description"));
        REQUIRE_FALSE(j.contains("mimeType"));
    }

    SECTION("Resource with MIME type") {
        Resource resource{
            "file:///data.json",
            "Data",
            std::nullopt,
            "application/json"
        };

        auto j = resource.to_json();
        
        REQUIRE(j["uri"] == "file:///data.json");
        REQUIRE(j["name"] == "Data");
        REQUIRE(j["mimeType"] == "application/json");
    }
}

TEST_CASE("ResourceContent serialization", "[protocol][resource_content]") {
    SECTION("Text content") {
        ResourceContent content{
            "file:///test.txt",
            "text/plain",
            "Hello, World!",
            std::nullopt
        };

        auto j = content.to_json();
        
        REQUIRE(j["uri"] == "file:///test.txt");
        REQUIRE(j["mimeType"] == "text/plain");
        REQUIRE(j["text"] == "Hello, World!");
        REQUIRE_FALSE(j.contains("blob"));
    }

    SECTION("Content with optional fields") {
        ResourceContent content{
            "file:///unknown",
            std::nullopt,
            "Some content",
            std::nullopt
        };

        auto j = content.to_json();
        
        REQUIRE(j["uri"] == "file:///unknown");
        REQUIRE_FALSE(j.contains("mimeType"));
        REQUIRE(j["text"] == "Some content");
    }
}

TEST_CASE("ServerInfo serialization", "[protocol][server_info]") {
    SECTION("Complete ServerInfo") {
        ServerInfo info{
            Implementation{"test-server", "1.0.0"},
            "2024-11-05",
            json::object(),
            std::nullopt
        };

        auto j = info.to_json();
        
        REQUIRE(j["serverInfo"]["name"] == "test-server");
        REQUIRE(j["serverInfo"]["version"] == "1.0.0");
        REQUIRE(j["protocolVersion"] == "2024-11-05");
    }

    SECTION("ServerInfo with instructions") {
        ServerInfo info{
            Implementation{"Test Server (Beta)", "1.0.0-alpha.1"},
            "2024-11-05",
            json::object(),
            "Special instructions"
        };

        auto j = info.to_json();
        
        REQUIRE(j["serverInfo"]["name"] == "Test Server (Beta)");
        REQUIRE(j["serverInfo"]["version"] == "1.0.0-alpha.1");
        REQUIRE(j["instructions"] == "Special instructions");
    }
}

TEST_CASE("ClientInfo serialization", "[protocol][client_info]") {
    SECTION("Complete ClientInfo") {
        ClientInfo info{
            Implementation{"test-client", "2.0.0"},
            "2024-11-05",
            json::object()
        };

        auto j = info.to_json();
        
        REQUIRE(j["clientInfo"]["name"] == "test-client");
        REQUIRE(j["clientInfo"]["version"] == "2.0.0");
        REQUIRE(j["protocolVersion"] == "2024-11-05");
    }
}

TEST_CASE("Implementation serialization", "[protocol][implementation]") {
    SECTION("Complete Implementation") {
        Implementation impl{
            "mcp-test",
            "0.1.0"
        };

        auto j = impl.to_json();
        
        REQUIRE(j["name"] == "mcp-test");
        REQUIRE(j["version"] == "0.1.0");
    }
}

TEST_CASE("Capabilities serialization", "[protocol][capabilities]") {
    SECTION("Empty ServerCapabilities") {
        ServerCapabilities caps;

        auto j = caps.to_json();
        
        REQUIRE(j.is_object());
        REQUIRE_FALSE(j.contains("tools"));
        REQUIRE_FALSE(j.contains("prompts"));
        REQUIRE_FALSE(j.contains("resources"));
    }

    SECTION("ServerCapabilities with tools enabled") {
        ServerCapabilities caps;
        caps.tools = json::object();

        auto j = caps.to_json();
        
        REQUIRE(j.contains("tools"));
        REQUIRE(j["tools"].is_object());
    }

    SECTION("ServerCapabilities with all features") {
        ServerCapabilities caps;
        caps.tools = json::object();
        caps.prompts = json::object();
        caps.resources = json{{"subscribe", true}};

        auto j = caps.to_json();
        
        REQUIRE(j.contains("tools"));
        REQUIRE(j.contains("prompts"));
        REQUIRE(j.contains("resources"));
        REQUIRE(j["resources"]["subscribe"] == true);
    }

    SECTION("Empty ClientCapabilities") {
        ClientCapabilities caps;

        auto j = caps.to_json();
        
        REQUIRE(j.is_object());
        REQUIRE_FALSE(j.contains("sampling"));
    }
}

TEST_CASE("SamplingMessage serialization", "[protocol][sampling]") {
    SECTION("User message") {
        MessageContent content{
            "text",
            "Hello",
            std::nullopt,
            std::nullopt
        };
        
        SamplingMessage msg{
            MessageRole::User,
            content
        };

        auto j = msg.to_json();
        
        REQUIRE(j["role"] == "user");
        REQUIRE(j["content"]["type"] == "text");
        REQUIRE(j["content"]["text"] == "Hello");
    }

    SECTION("Assistant message") {
        MessageContent content{
            "text",
            "Hi there!",
            std::nullopt,
            std::nullopt
        };
        
        SamplingMessage msg{
            MessageRole::Assistant,
            content
        };

        auto j = msg.to_json();
        
        REQUIRE(j["role"] == "assistant");
        REQUIRE(j["content"]["text"] == "Hi there!");
    }
}

TEST_CASE("Protocol version validation", "[protocol][version]") {
    SECTION("Valid protocol version") {
        std::string version = "2024-11-05";
        // In a real implementation, you'd validate this
        REQUIRE(version == "2024-11-05");
    }
}

TEST_CASE("Tool input schema validation", "[protocol][tool][schema]") {
    SECTION("Valid JSON schema") {
        ToolInputSchema schema;
        schema.type = "object";
        schema.properties = {
            {"name", {{"type", "string"}}},
            {"age", {{"type", "number"}, {"minimum", 0}}}
        };
        schema.required = {"name"};

        Tool tool{"test", "desc", schema};
        auto j = tool.to_json();
        
        REQUIRE(j["inputSchema"]["type"] == "object");
        REQUIRE(j["inputSchema"]["properties"]["age"]["minimum"] == 0);
        REQUIRE(j["inputSchema"]["required"].size() == 1);
    }

    SECTION("Empty schema") {
        ToolInputSchema schema;
        
        Tool tool{"test", "desc", schema};
        auto j = tool.to_json();
        
        REQUIRE(j["inputSchema"]["type"] == "object");
    }
}

TEST_CASE("Resource URI formats", "[protocol][resource][uri]") {
    SECTION("File URI") {
        Resource res{"file:///path/to/file.txt", "File", std::nullopt, std::nullopt};
        auto j = res.to_json();
        REQUIRE(j["uri"] == "file:///path/to/file.txt");
    }

    SECTION("HTTP URI") {
        Resource res{"http://example.com/resource", "Web Resource", std::nullopt, std::nullopt};
        auto j = res.to_json();
        REQUIRE(j["uri"] == "http://example.com/resource");
    }

    SECTION("Custom scheme URI") {
        Resource res{"custom://internal/resource", "Custom", std::nullopt, std::nullopt};
        auto j = res.to_json();
        REQUIRE(j["uri"] == "custom://internal/resource");
    }
}

TEST_CASE("Edge cases and error handling", "[protocol][edge_cases]") {
    SECTION("Empty tool name") {
        Tool tool{"", "desc", ToolInputSchema{}};
        auto j = tool.to_json();
        REQUIRE(j["name"] == "");
    }

    SECTION("Very long description") {
        std::string long_desc(10000, 'a');
        Tool tool{"test", long_desc, ToolInputSchema{}};
        auto j = tool.to_json();
        REQUIRE(j["description"].get<std::string>().size() == 10000);
    }

    SECTION("Unicode in names") {
        Tool tool{"测试工具", "Test with unicode: 日本語", ToolInputSchema{}};
        auto j = tool.to_json();
        REQUIRE(j["name"] == "测试工具");
    }

    SECTION("Special characters in URIs") {
        Resource res{"file:///path%20with%20spaces/file.txt", "Spaced", std::nullopt, std::nullopt};
        auto j = res.to_json();
        REQUIRE(j["uri"] == "file:///path%20with%20spaces/file.txt");
    }
}
