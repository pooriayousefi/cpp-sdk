#pragma once

#include "protocol.hpp"
#include "jsonrpc/jsonrpc.hpp"
#include "transport/transport.hpp"
#include <memory>
#include <functional>
#include <vector>
#include <stdexcept>

/**
 * @file client.hpp
 * @brief MCP Client implementation
 * 
 * Provides a high-level client for connecting to MCP servers.
 * Built on top of the JSON-RPC endpoint with MCP-specific protocol handling.
 */

namespace pooriayousefi::mcp 
{
    /**
     * @brief MCP Client for connecting to servers and using their capabilities
     */
    class Client 
    {
    public:
        using ErrorCallback = std::function<void(const std::string&)>;
        using InitializeCallback = std::function<void(const ServerInfo&)>;
        using ToolsCallback = std::function<void(const std::vector<Tool>&)>;
        using PromptsCallback = std::function<void(const std::vector<Prompt>&)>;
        using ResourcesCallback = std::function<void(const std::vector<Resource>&)>;
        using ToolResultCallback = std::function<void(const std::vector<ToolResultContent>&)>;

        /**
         * @brief Construct client with transport
         * @param transport Transport layer (stdio, http, etc.)
         */
        explicit Client(std::shared_ptr<transport::Transport> transport)
            : transport_(std::move(transport))
            , endpoint_(std::make_unique<jsonrpc::endpoint>([this](const json& msg) {
                  transport_->send(msg);
              }))
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
        }

        /**
         * @brief Start the client (begins transport)
         */
        void start() {
            transport_->start();
        }

        /**
         * @brief Stop the client and close transport
         */
        void close() {
            transport_->close();
            initialized_ = false;
        }

        /**
         * @brief Check if client is initialized
         */
        bool is_initialized() const {
            return initialized_;
        }

        /**
         * @brief Set error callback
         */
        void on_error(ErrorCallback callback) {
            error_callback_ = std::move(callback);
        }

        /**
         * @brief Initialize connection with server
         * @param client_info Client implementation info
         * @param capabilities Client capabilities
         * @param on_success Callback on successful initialization
         * @param on_error Callback on error
         */
        void initialize(
            const Implementation& client_info,
            const ClientCapabilities& capabilities,
            InitializeCallback on_success,
            ErrorCallback on_error
        ) {
            json params = {
                {"protocolVersion", MCP_PROTOCOL_VERSION},
                {"capabilities", capabilities.to_json()},
                {"clientInfo", client_info.to_json()}
            };

            endpoint_->send_request(
                "initialize",
                params,
                [this, on_success](const json& result) {
                    server_info_ = ServerInfo::from_json(result);
                    initialized_ = true;
                    
                    // Send initialized notification
                    endpoint_->send_notification("notifications/initialized", json::object());
                    
                    if (on_success) {
                        on_success(server_info_);
                    }
                },
                [on_error](const json& error) {
                    if (on_error) {
                        std::string msg = error.value("message", "Unknown error");
                        on_error(msg);
                    }
                }
            );
        }

        /**
         * @brief List available tools from server
         * @param on_success Callback with list of tools
         * @param on_error Callback on error
         */
        void list_tools(ToolsCallback on_success, ErrorCallback on_error) {
            if (!initialized_) {
                if (on_error) on_error("Client not initialized");
                return;
            }

            endpoint_->send_request(
                "tools/list",
                json::object(),
                [on_success](const json& result) {
                    std::vector<Tool> tools;
                    if (result.contains("tools") && result["tools"].is_array()) {
                        for (const auto& tool_json : result["tools"]) {
                            tools.push_back(Tool::from_json(tool_json));
                        }
                    }
                    if (on_success) on_success(tools);
                },
                [on_error](const json& error) {
                    if (on_error) {
                        std::string msg = error.value("message", "Unknown error");
                        on_error(msg);
                    }
                }
            );
        }

        /**
         * @brief Call a tool on the server
         * @param tool_name Name of the tool to call
         * @param arguments Tool arguments as JSON
         * @param on_success Callback with tool result
         * @param on_error Callback on error
         */
        void call_tool(
            const std::string& tool_name,
            const json& arguments,
            ToolResultCallback on_success,
            ErrorCallback on_error
        ) {
            if (!initialized_) {
                if (on_error) on_error("Client not initialized");
                return;
            }

            json params = {
                {"name", tool_name},
                {"arguments", arguments}
            };

            endpoint_->send_request(
                "tools/call",
                params,
                [on_success](const json& result) {
                    std::vector<ToolResultContent> contents;
                    if (result.contains("content") && result["content"].is_array()) {
                        for (const auto& content_json : result["content"]) {
                            ToolResultContent content;
                            content.type = content_json.value("type", "text");
                            if (content_json.contains("text")) {
                                content.text = content_json["text"].get<std::string>();
                            }
                            if (content_json.contains("data")) {
                                content.data = content_json["data"].get<std::string>();
                            }
                            if (content_json.contains("mimeType")) {
                                content.mime_type = content_json["mimeType"].get<std::string>();
                            }
                            contents.push_back(content);
                        }
                    }
                    if (on_success) on_success(contents);
                },
                [on_error](const json& error) {
                    if (on_error) {
                        std::string msg = error.value("message", "Unknown error");
                        on_error(msg);
                    }
                }
            );
        }

        /**
         * @brief List available prompts from server
         * @param on_success Callback with list of prompts
         * @param on_error Callback on error
         */
        void list_prompts(PromptsCallback on_success, ErrorCallback on_error) {
            if (!initialized_) {
                if (on_error) on_error("Client not initialized");
                return;
            }

            endpoint_->send_request(
                "prompts/list",
                json::object(),
                [on_success](const json& result) {
                    std::vector<Prompt> prompts;
                    if (result.contains("prompts") && result["prompts"].is_array()) {
                        for (const auto& prompt_json : result["prompts"]) {
                            prompts.push_back(Prompt::from_json(prompt_json));
                        }
                    }
                    if (on_success) on_success(prompts);
                },
                [on_error](const json& error) {
                    if (on_error) {
                        std::string msg = error.value("message", "Unknown error");
                        on_error(msg);
                    }
                }
            );
        }

        /**
         * @brief Get a prompt with arguments
         * @param prompt_name Name of the prompt
         * @param arguments Prompt arguments as key-value map
         * @param on_success Callback with prompt messages
         * @param on_error Callback on error
         */
        void get_prompt(
            const std::string& prompt_name,
            const std::map<std::string, std::string>& arguments,
            std::function<void(const std::vector<PromptMessage>&)> on_success,
            ErrorCallback on_error
        ) {
            if (!initialized_) {
                if (on_error) on_error("Client not initialized");
                return;
            }

            json args = json::object();
            for (const auto& [key, value] : arguments) {
                args[key] = value;
            }

            json params = {
                {"name", prompt_name},
                {"arguments", args}
            };

            endpoint_->send_request(
                "prompts/get",
                params,
                [on_success](const json& result) {
                    std::vector<PromptMessage> messages;
                    if (result.contains("messages") && result["messages"].is_array()) {
                        for (const auto& msg_json : result["messages"]) {
                            PromptMessage msg;
                            std::string role_str = msg_json.value("role", "user");
                            msg.role = (role_str == "assistant") ? MessageRole::Assistant : MessageRole::User;
                            
                            if (msg_json.contains("content") && msg_json["content"].is_object()) {
                                MessageContent content;
                                content.type = msg_json["content"].value("type", "text");
                                if (msg_json["content"].contains("text")) {
                                    content.text = msg_json["content"]["text"].get<std::string>();
                                }
                                msg.content.push_back(content);
                            }
                            messages.push_back(msg);
                        }
                    }
                    if (on_success) on_success(messages);
                },
                [on_error](const json& error) {
                    if (on_error) {
                        std::string msg = error.value("message", "Unknown error");
                        on_error(msg);
                    }
                }
            );
        }

        /**
         * @brief List available resources from server
         * @param on_success Callback with list of resources
         * @param on_error Callback on error
         */
        void list_resources(ResourcesCallback on_success, ErrorCallback on_error) {
            if (!initialized_) {
                if (on_error) on_error("Client not initialized");
                return;
            }

            endpoint_->send_request(
                "resources/list",
                json::object(),
                [on_success](const json& result) {
                    std::vector<Resource> resources;
                    if (result.contains("resources") && result["resources"].is_array()) {
                        for (const auto& res_json : result["resources"]) {
                            resources.push_back(Resource::from_json(res_json));
                        }
                    }
                    if (on_success) on_success(resources);
                },
                [on_error](const json& error) {
                    if (on_error) {
                        std::string msg = error.value("message", "Unknown error");
                        on_error(msg);
                    }
                }
            );
        }

        /**
         * @brief Read a resource from server
         * @param uri Resource URI
         * @param on_success Callback with resource contents
         * @param on_error Callback on error
         */
        void read_resource(
            const std::string& uri,
            std::function<void(const std::vector<ResourceContent>&)> on_success,
            ErrorCallback on_error
        ) {
            if (!initialized_) {
                if (on_error) on_error("Client not initialized");
                return;
            }

            json params = {{"uri", uri}};

            endpoint_->send_request(
                "resources/read",
                params,
                [on_success](const json& result) {
                    std::vector<ResourceContent> contents;
                    if (result.contains("contents") && result["contents"].is_array()) {
                        for (const auto& content_json : result["contents"]) {
                            ResourceContent content;
                            content.uri = content_json.value("uri", "");
                            if (content_json.contains("mimeType")) {
                                content.mime_type = content_json["mimeType"].get<std::string>();
                            }
                            if (content_json.contains("text")) {
                                content.text = content_json["text"].get<std::string>();
                            }
                            if (content_json.contains("blob")) {
                                content.blob = content_json["blob"].get<std::string>();
                            }
                            contents.push_back(content);
                        }
                    }
                    if (on_success) on_success(contents);
                },
                [on_error](const json& error) {
                    if (on_error) {
                        std::string msg = error.value("message", "Unknown error");
                        on_error(msg);
                    }
                }
            );
        }

        /**
         * @brief Get server info (available after initialization)
         */
        const ServerInfo& server_info() const {
            if (!initialized_) {
                throw std::runtime_error("Client not initialized");
            }
            return server_info_;
        }

    private:
        std::shared_ptr<transport::Transport> transport_;
        std::unique_ptr<jsonrpc::endpoint> endpoint_;
        ServerInfo server_info_;
        bool initialized_;
        ErrorCallback error_callback_;
    };

} // namespace pooriayousefi::mcp
