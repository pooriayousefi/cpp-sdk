#pragma once

#include "protocol.hpp"
#include "jsonrpc/jsonrpc.hpp"
#include "transport/transport.hpp"
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include <stdexcept>

/**
 * @file server.hpp
 * @brief MCP Server implementation
 * 
 * Provides a high-level server for hosting MCP capabilities.
 * Built on top of the JSON-RPC endpoint with MCP-specific protocol handling.
 */

namespace pooriayousefi::mcp 
{
    /**
     * @brief MCP Server for providing tools, prompts, and resources
     */
    class Server 
    {
    public:
        using ToolHandler = std::function<std::vector<ToolResultContent>(const json& arguments)>;
        using PromptHandler = std::function<std::vector<PromptMessage>(const std::map<std::string, std::string>& arguments)>;
        using ResourceReader = std::function<std::vector<ResourceContent>(const std::string& uri)>;
        using ErrorCallback = std::function<void(const std::string&)>;

        /**
         * @brief Construct server with transport and server info
         * @param transport Transport layer (stdio, http, etc.)
         * @param server_info Server implementation information
         */
        Server(
            std::shared_ptr<transport::Transport> transport,
            const Implementation& server_info
        )
            : transport_(std::move(transport))
            , endpoint_(std::make_unique<jsonrpc::endpoint>([this](const json& msg) {
                  transport_->send(msg);
              }))
            , server_info_(server_info)
            , initialized_(false)
        {
            // Set up transport message handler
            transport_->on_message([this](const json& msg) {
                endpoint_->receive(msg);
            });

            transport_->on_error([this](const std::string& error) {
                if (error_callback_) {
                    error_callback_(error);
                }
            });

            transport_->on_close([this]() {
                initialized_ = false;
            });

            // Register MCP protocol methods
            register_protocol_methods();
        }

        /**
         * @brief Set server instructions (shown to LLMs)
         */
        void set_instructions(const std::string& instructions) {
            instructions_ = instructions;
        }

        /**
         * @brief Enable tool capability
         * @param list_changed_notifications Enable notifications when tool list changes
         */
        void enable_tools(bool list_changed_notifications = false) {
            capabilities_.tools = json{{"listChanged", list_changed_notifications}};
        }

        /**
         * @brief Enable prompts capability
         * @param list_changed_notifications Enable notifications when prompt list changes
         */
        void enable_prompts(bool list_changed_notifications = false) {
            capabilities_.prompts = json{{"listChanged", list_changed_notifications}};
        }

        /**
         * @brief Enable resources capability
         * @param subscribe Enable resource subscriptions
         * @param list_changed_notifications Enable notifications when resource list changes
         */
        void enable_resources(bool subscribe = false, bool list_changed_notifications = false) {
            capabilities_.resources = json{
                {"subscribe", subscribe},
                {"listChanged", list_changed_notifications}
            };
        }

        /**
         * @brief Enable logging capability
         */
        void enable_logging() {
            capabilities_.logging = json::object();
        }

        /**
         * @brief Register a tool
         * @param tool Tool definition
         * @param handler Handler function that processes tool calls
         */
        void register_tool(const Tool& tool, ToolHandler handler) {
            tools_[tool.name] = tool;
            tool_handlers_[tool.name] = std::move(handler);
        }

        /**
         * @brief Register a prompt
         * @param prompt Prompt definition
         * @param handler Handler function that generates prompt messages
         */
        void register_prompt(const Prompt& prompt, PromptHandler handler) {
            prompts_[prompt.name] = prompt;
            prompt_handlers_[prompt.name] = std::move(handler);
        }

        /**
         * @brief Register a resource
         * @param resource Resource definition
         * @param reader Reader function that provides resource content
         */
        void register_resource(const Resource& resource, ResourceReader reader) {
            resources_[resource.uri] = resource;
            resource_readers_[resource.uri] = std::move(reader);
        }

        /**
         * @brief Set error callback
         */
        void on_error(ErrorCallback callback) {
            error_callback_ = std::move(callback);
        }

        /**
         * @brief Start the server (begins transport)
         */
        void start() {
            transport_->start();
        }

        /**
         * @brief Stop the server and close transport
         */
        void close() {
            transport_->close();
            initialized_ = false;
        }

        /**
         * @brief Check if server is initialized
         */
        bool is_initialized() const {
            return initialized_;
        }

        /**
         * @brief Send a notification to client
         */
        void send_notification(const std::string& method, const json& params = json::object()) {
            endpoint_->send_notification(method, params);
        }

        /**
         * @brief Notify that tools list has changed
         */
        void notify_tools_changed() {
            send_notification("notifications/tools/list_changed");
        }

        /**
         * @brief Notify that prompts list has changed
         */
        void notify_prompts_changed() {
            send_notification("notifications/prompts/list_changed");
        }

        /**
         * @brief Notify that resources list has changed
         */
        void notify_resources_changed() {
            send_notification("notifications/resources/list_changed");
        }

        /**
         * @brief Send a log message to client
         * @param level Log level ("debug", "info", "warning", "error")
         * @param data Log data
         */
        void send_log(const std::string& level, const json& data) {
            json params = {
                {"level", level},
                {"data", data}
            };
            send_notification("notifications/message", params);
        }

    private:
        void register_protocol_methods() {
            // Initialize
            endpoint_->add("initialize", [this](const json& params) -> json {
                if (initialized_) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32600, "Already initialized", nullptr
                    });
                }

                // Store client capabilities
                if (params.contains("capabilities")) {
                    client_capabilities_ = params["capabilities"];
                }

                initialized_ = true;

                // Return server info and capabilities
                ServerInfo info{
                    server_info_,
                    MCP_PROTOCOL_VERSION,
                    capabilities_.to_json(),
                    instructions_
                };

                return info.to_json();
            });

            // Tools list
            endpoint_->add("tools/list", [this](const json&) -> json {
                if (!initialized_) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32600, "Not initialized", nullptr
                    });
                }

                json tools_array = json::array();
                for (const auto& [name, tool] : tools_) {
                    tools_array.push_back(tool.to_json());
                }

                return json{{"tools", tools_array}};
            });

            // Tools call
            endpoint_->add("tools/call", [this](const json& params) -> json {
                if (!initialized_) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32600, "Not initialized", nullptr
                    });
                }

                std::string tool_name = params.value("name", "");
                if (tool_name.empty()) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32602, "Missing tool name", nullptr
                    });
                }

                auto it = tool_handlers_.find(tool_name);
                if (it == tool_handlers_.end()) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32601, "Tool not found: " + tool_name, nullptr
                    });
                }

                json arguments = params.value("arguments", json::object());
                
                try {
                    auto result = it->second(arguments);
                    
                    json content_array = json::array();
                    for (const auto& content : result) {
                        content_array.push_back(content.to_json());
                    }

                    return json{{"content", content_array}};
                } catch (const std::exception& e) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32603, std::string("Tool execution failed: ") + e.what(), nullptr
                    });
                }
            });

            // Prompts list
            endpoint_->add("prompts/list", [this](const json&) -> json {
                if (!initialized_) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32600, "Not initialized", nullptr
                    });
                }

                json prompts_array = json::array();
                for (const auto& [name, prompt] : prompts_) {
                    prompts_array.push_back(prompt.to_json());
                }

                return json{{"prompts", prompts_array}};
            });

            // Prompts get
            endpoint_->add("prompts/get", [this](const json& params) -> json {
                if (!initialized_) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32600, "Not initialized", nullptr
                    });
                }

                std::string prompt_name = params.value("name", "");
                if (prompt_name.empty()) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32602, "Missing prompt name", nullptr
                    });
                }

                auto it = prompt_handlers_.find(prompt_name);
                if (it == prompt_handlers_.end()) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32601, "Prompt not found: " + prompt_name, nullptr
                    });
                }

                // Extract arguments
                std::map<std::string, std::string> arguments;
                if (params.contains("arguments") && params["arguments"].is_object()) {
                    for (auto& [key, value] : params["arguments"].items()) {
                        if (value.is_string()) {
                            arguments[key] = value.get<std::string>();
                        }
                    }
                }

                try {
                    auto messages = it->second(arguments);
                    
                    json messages_array = json::array();
                    for (const auto& msg : messages) {
                        messages_array.push_back(msg.to_json());
                    }

                    return json{{"messages", messages_array}};
                } catch (const std::exception& e) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32603, std::string("Prompt generation failed: ") + e.what(), nullptr
                    });
                }
            });

            // Resources list
            endpoint_->add("resources/list", [this](const json&) -> json {
                if (!initialized_) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32600, "Not initialized", nullptr
                    });
                }

                json resources_array = json::array();
                for (const auto& [uri, resource] : resources_) {
                    resources_array.push_back(resource.to_json());
                }

                return json{{"resources", resources_array}};
            });

            // Resources read
            endpoint_->add("resources/read", [this](const json& params) -> json {
                if (!initialized_) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32600, "Not initialized", nullptr
                    });
                }

                std::string uri = params.value("uri", "");
                if (uri.empty()) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32602, "Missing resource URI", nullptr
                    });
                }

                auto it = resource_readers_.find(uri);
                if (it == resource_readers_.end()) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32601, "Resource not found: " + uri, nullptr
                    });
                }

                try {
                    auto contents = it->second(uri);
                    
                    json contents_array = json::array();
                    for (const auto& content : contents) {
                        contents_array.push_back(content.to_json());
                    }

                    return json{{"contents", contents_array}};
                } catch (const std::exception& e) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32603, std::string("Resource read failed: ") + e.what(), nullptr
                    });
                }
            });
        }

        std::shared_ptr<transport::Transport> transport_;
        std::unique_ptr<jsonrpc::endpoint> endpoint_;
        Implementation server_info_;
        std::optional<std::string> instructions_;
        ServerCapabilities capabilities_;
        json client_capabilities_;
        bool initialized_;
        ErrorCallback error_callback_;

        // Registry
        std::unordered_map<std::string, Tool> tools_;
        std::unordered_map<std::string, ToolHandler> tool_handlers_;
        std::unordered_map<std::string, Prompt> prompts_;
        std::unordered_map<std::string, PromptHandler> prompt_handlers_;
        std::unordered_map<std::string, Resource> resources_;
        std::unordered_map<std::string, ResourceReader> resource_readers_;
    };

} // namespace pooriayousefi::mcp
