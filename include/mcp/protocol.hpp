#pragma once

#include "jsonrpc/jsonrpc.hpp"
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <map>

/**
 * @file protocol.hpp
 * @brief Model Context Protocol (MCP) specification types and structures
 * 
 * This file defines all MCP protocol types according to the official specification:
 * https://spec.modelcontextprotocol.io/specification/2024-11-05/
 * 
 * MCP is built on JSON-RPC 2.0 and defines:
 * - Client-Server initialization and capability negotiation
 * - Resources: Application context shared with LLMs (files, data, API responses)
 * - Prompts: Pre-written templates for LLM interactions
 * - Tools: Functions LLMs can invoke to perform actions
 * - Sampling: Server-initiated LLM completions
 */

namespace pooriayousefi::mcp 
{
    using json = jsonrpc::json;

    // ==================== Core Types ====================

    /**
     * @brief Implementation information
     */
    struct Implementation 
    {
        std::string name;
        std::string version;
        
        json to_json() const {
            return json{{"name", name}, {"version", version}};
        }
        
        static Implementation from_json(const json& j) {
            return Implementation{
                j.value("name", ""),
                j.value("version", "")
            };
        }
    };

    /**
     * @brief Client information sent in initialize request
     */
    struct ClientInfo 
    {
        Implementation client_info;
        std::string protocol_version;
        json capabilities;
        
        json to_json() const {
            return json{
                {"protocolVersion", protocol_version},
                {"capabilities", capabilities},
                {"clientInfo", client_info.to_json()}
            };
        }
    };

    /**
     * @brief Server information sent in initialize response
     */
    struct ServerInfo 
    {
        Implementation server_info;
        std::string protocol_version;
        json capabilities;
        std::optional<std::string> instructions;
        
        json to_json() const {
            json j = {
                {"protocolVersion", protocol_version},
                {"capabilities", capabilities},
                {"serverInfo", server_info.to_json()}
            };
            if (instructions) j["instructions"] = *instructions;
            return j;
        }
        
        static ServerInfo from_json(const json& j) {
            return ServerInfo{
                Implementation::from_json(j.value("serverInfo", json::object())),
                j.value("protocolVersion", ""),
                j.value("capabilities", json::object()),
                j.contains("instructions") ? std::optional<std::string>(j["instructions"]) : std::nullopt
            };
        }
    };

    // ==================== Tool Types ====================

    /**
     * @brief JSON Schema for tool input parameters
     */
    struct ToolInputSchema 
    {
        std::string type = "object";
        json properties;
        std::vector<std::string> required;
        
        json to_json() const {
            json j = {{"type", type}};
            if (!properties.is_null()) j["properties"] = properties;
            if (!required.empty()) j["required"] = required;
            return j;
        }
        
        static ToolInputSchema from_json(const json& j) {
            ToolInputSchema schema;
            schema.type = j.value("type", "object");
            schema.properties = j.value("properties", json::object());
            if (j.contains("required") && j["required"].is_array()) {
                for (const auto& r : j["required"]) {
                    schema.required.push_back(r.get<std::string>());
                }
            }
            return schema;
        }
    };

    /**
     * @brief Tool definition
     */
    struct Tool 
    {
        std::string name;
        std::optional<std::string> description;
        ToolInputSchema input_schema;
        
        json to_json() const {
            json j = {
                {"name", name},
                {"inputSchema", input_schema.to_json()}
            };
            if (description) j["description"] = *description;
            return j;
        }
        
        static Tool from_json(const json& j) {
            return Tool{
                j.value("name", ""),
                j.contains("description") ? std::optional<std::string>(j["description"]) : std::nullopt,
                ToolInputSchema::from_json(j.value("inputSchema", json::object()))
            };
        }
    };

    /**
     * @brief Tool execution result content (text or image or resource)
     */
    struct ToolResultContent 
    {
        std::string type; // "text", "image", "resource"
        std::optional<std::string> text;
        std::optional<std::string> data; // base64 for images
        std::optional<std::string> mime_type;
        std::optional<std::string> uri; // for resources
        
        json to_json() const {
            json j = {{"type", type}};
            if (text) j["text"] = *text;
            if (data) j["data"] = *data;
            if (mime_type) j["mimeType"] = *mime_type;
            if (uri) j["uri"] = *uri;
            return j;
        }
        
        static ToolResultContent text_content(const std::string& text) {
            return ToolResultContent{"text", text, std::nullopt, std::nullopt, std::nullopt};
        }
    };

    // ==================== Prompt Types ====================

    /**
     * @brief Prompt argument definition
     */
    struct PromptArgument 
    {
        std::string name;
        std::optional<std::string> description;
        bool required = false;
        
        json to_json() const {
            json j = {
                {"name", name},
                {"required", required}
            };
            if (description) j["description"] = *description;
            return j;
        }
        
        static PromptArgument from_json(const json& j) {
            return PromptArgument{
                j.value("name", ""),
                j.contains("description") ? std::optional<std::string>(j["description"]) : std::nullopt,
                j.value("required", false)
            };
        }
    };

    /**
     * @brief Prompt definition
     */
    struct Prompt 
    {
        std::string name;
        std::optional<std::string> description;
        std::vector<PromptArgument> arguments;
        
        json to_json() const {
            json j = {{"name", name}};
            if (description) j["description"] = *description;
            if (!arguments.empty()) {
                json args = json::array();
                for (const auto& arg : arguments) {
                    args.push_back(arg.to_json());
                }
                j["arguments"] = args;
            }
            return j;
        }
        
        static Prompt from_json(const json& j) {
            Prompt p;
            p.name = j.value("name", "");
            if (j.contains("description")) p.description = j["description"];
            if (j.contains("arguments") && j["arguments"].is_array()) {
                for (const auto& arg : j["arguments"]) {
                    p.arguments.push_back(PromptArgument::from_json(arg));
                }
            }
            return p;
        }
    };

    /**
     * @brief Message role in prompts
     */
    enum class MessageRole {
        User,
        Assistant
    };

    inline std::string message_role_to_string(MessageRole role) {
        return role == MessageRole::User ? "user" : "assistant";
    }

    /**
     * @brief Message content (text or image)
     */
    struct MessageContent 
    {
        std::string type; // "text" or "image"
        std::optional<std::string> text;
        std::optional<std::string> data; // base64 for images
        std::optional<std::string> mime_type;
        
        json to_json() const {
            json j = {{"type", type}};
            if (text) j["text"] = *text;
            if (data) j["data"] = *data;
            if (mime_type) j["mimeType"] = *mime_type;
            return j;
        }
    };

    /**
     * @brief Prompt message
     */
    struct PromptMessage 
    {
        MessageRole role;
        std::vector<MessageContent> content;
        
        json to_json() const {
            json contents = json::array();
            for (const auto& c : content) {
                contents.push_back(c.to_json());
            }
            return json{
                {"role", message_role_to_string(role)},
                {"content", contents}
            };
        }
    };

    // ==================== Resource Types ====================

    /**
     * @brief Resource content (text or blob)
     */
    struct ResourceContent 
    {
        std::string uri;
        std::optional<std::string> mime_type;
        std::optional<std::string> text;
        std::optional<std::string> blob; // base64
        
        json to_json() const {
            json j = {{"uri", uri}};
            if (mime_type) j["mimeType"] = *mime_type;
            if (text) j["text"] = *text;
            if (blob) j["blob"] = *blob;
            return j;
        }
    };

    /**
     * @brief Resource definition
     */
    struct Resource 
    {
        std::string uri;
        std::string name;
        std::optional<std::string> description;
        std::optional<std::string> mime_type;
        
        json to_json() const {
            json j = {
                {"uri", uri},
                {"name", name}
            };
            if (description) j["description"] = *description;
            if (mime_type) j["mimeType"] = *mime_type;
            return j;
        }
        
        static Resource from_json(const json& j) {
            return Resource{
                j.value("uri", ""),
                j.value("name", ""),
                j.contains("description") ? std::optional<std::string>(j["description"]) : std::nullopt,
                j.contains("mimeType") ? std::optional<std::string>(j["mimeType"]) : std::nullopt
            };
        }
    };

    /**
     * @brief Resource template for URI patterns
     */
    struct ResourceTemplate 
    {
        std::string uri_template;
        std::string name;
        std::optional<std::string> description;
        std::optional<std::string> mime_type;
        
        json to_json() const {
            json j = {
                {"uriTemplate", uri_template},
                {"name", name}
            };
            if (description) j["description"] = *description;
            if (mime_type) j["mimeType"] = *mime_type;
            return j;
        }
    };

    // ==================== Sampling Types ====================

    /**
     * @brief Sampling message for LLM completion
     */
    struct SamplingMessage 
    {
        MessageRole role;
        MessageContent content;
        
        json to_json() const {
            return json{
                {"role", message_role_to_string(role)},
                {"content", content.to_json()}
            };
        }
    };

    /**
     * @brief Model preferences for sampling
     */
    struct ModelPreferences 
    {
        std::vector<std::string> hints; // Suggested models
        std::optional<double> cost_priority; // 0-1, higher = prefer cost savings
        std::optional<double> speed_priority; // 0-1, higher = prefer speed
        std::optional<double> intelligence_priority; // 0-1, higher = prefer capability
        
        json to_json() const {
            json j;
            if (!hints.empty()) j["hints"] = hints;
            if (cost_priority) j["costPriority"] = *cost_priority;
            if (speed_priority) j["speedPriority"] = *speed_priority;
            if (intelligence_priority) j["intelligencePriority"] = *intelligence_priority;
            return j;
        }
    };

    // ==================== Capability Types ====================

    /**
     * @brief Server capabilities
     */
    struct ServerCapabilities 
    {
        std::optional<json> prompts;
        std::optional<json> resources;
        std::optional<json> tools;
        std::optional<json> logging;
        
        json to_json() const {
            json j = json::object();
            if (prompts) j["prompts"] = *prompts;
            if (resources) j["resources"] = *resources;
            if (tools) j["tools"] = *tools;
            if (logging) j["logging"] = *logging;
            return j;
        }
    };

    /**
     * @brief Client capabilities
     */
    struct ClientCapabilities 
    {
        std::optional<json> experimental;
        std::optional<json> sampling;
        std::optional<json> roots;
        
        json to_json() const {
            json j = json::object();
            if (experimental) j["experimental"] = *experimental;
            if (sampling) j["sampling"] = *sampling;
            if (roots) j["roots"] = *roots;
            return j;
        }
    };

    // ==================== Notification Types ====================

    /**
     * @brief Progress notification token
     */
    struct ProgressToken 
    {
        std::variant<std::string, int64_t> token;
        
        json to_json() const {
            return std::visit([](auto&& v) -> json { return v; }, token);
        }
    };

    /**
     * @brief Progress notification
     */
    struct ProgressNotification 
    {
        ProgressToken progress_token;
        double progress; // 0.0 to 1.0
        std::optional<double> total;
        
        json to_json() const {
            json j = {
                {"progressToken", progress_token.to_json()},
                {"progress", progress}
            };
            if (total) j["total"] = *total;
            return j;
        }
    };

    // ==================== MCP Protocol Version ====================

    constexpr const char* MCP_PROTOCOL_VERSION = "2024-11-05";

} // namespace pooriayousefi::mcp
