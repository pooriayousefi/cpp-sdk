#pragma once

#include "client.hpp"
#include "core/asyncops.hpp"
#include <future>
#include <memory>

/**
 * @file client_async.hpp
 * @brief Async MCP Client using Task<T> coroutines
 * 
 * Provides non-blocking, coroutine-based client operations using
 * the Task<T> from asyncops.hpp for clean async/await syntax.
 */

namespace pooriayousefi::mcp 
{
    using core::Task;

    /**
     * @brief Async wrapper for MCP Client
     * 
     * Converts callback-based Client methods to Task<T>-based async methods.
     * Enables clean async/await syntax for non-blocking operations.
     * 
     * @example
     * ```cpp
     * AsyncClient async_client(client);
     * auto tools = co_await async_client.list_tools_async();
     * auto result = co_await async_client.call_tool_async("my_tool", args);
     * ```
     */
    class AsyncClient 
    {
    public:
        /**
         * @brief Construct async client wrapper
         * @param client Reference to underlying synchronous client
         */
        explicit AsyncClient(Client& client) : client_(client) {}

        /**
         * @brief Async initialize connection with server
         * @param client_info Client implementation info
         * @param capabilities Client capabilities
         * @return Task that resolves to ServerInfo on success
         * @throws std::runtime_error on initialization failure
         */
        Task<ServerInfo> initialize_async(
            const Implementation& client_info,
            const ClientCapabilities& capabilities
        ) {
            auto promise = std::make_shared<std::promise<ServerInfo>>();
            auto future = promise->get_future();

            client_.initialize(
                client_info,
                capabilities,
                [promise](const ServerInfo& info) {
                    promise->set_value(info);
                },
                [promise](const std::string& error) {
                    promise->set_exception(std::make_exception_ptr(
                        std::runtime_error(error)
                    ));
                }
            );

            co_return future.get();
        }

        /**
         * @brief Async list available tools from server
         * @return Task that resolves to vector of Tool definitions
         * @throws std::runtime_error on error
         */
        Task<std::vector<Tool>> list_tools_async() {
            auto promise = std::make_shared<std::promise<std::vector<Tool>>>();
            auto future = promise->get_future();

            client_.list_tools(
                [promise](const std::vector<Tool>& tools) {
                    promise->set_value(tools);
                },
                [promise](const std::string& error) {
                    promise->set_exception(std::make_exception_ptr(
                        std::runtime_error(error)
                    ));
                }
            );

            co_return future.get();
        }

        /**
         * @brief Async call a tool on the server
         * @param tool_name Name of the tool to call
         * @param arguments Tool arguments as JSON
         * @return Task that resolves to tool result contents
         * @throws std::runtime_error on error
         */
        Task<std::vector<ToolResultContent>> call_tool_async(
            const std::string& tool_name,
            const json& arguments
        ) {
            auto promise = std::make_shared<std::promise<std::vector<ToolResultContent>>>();
            auto future = promise->get_future();

            client_.call_tool(
                tool_name,
                arguments,
                [promise](const std::vector<ToolResultContent>& result) {
                    promise->set_value(result);
                },
                [promise](const std::string& error) {
                    promise->set_exception(std::make_exception_ptr(
                        std::runtime_error(error)
                    ));
                }
            );

            co_return future.get();
        }

        /**
         * @brief Async list available prompts from server
         * @return Task that resolves to vector of Prompt definitions
         * @throws std::runtime_error on error
         */
        Task<std::vector<Prompt>> list_prompts_async() {
            auto promise = std::make_shared<std::promise<std::vector<Prompt>>>();
            auto future = promise->get_future();

            client_.list_prompts(
                [promise](const std::vector<Prompt>& prompts) {
                    promise->set_value(prompts);
                },
                [promise](const std::string& error) {
                    promise->set_exception(std::make_exception_ptr(
                        std::runtime_error(error)
                    ));
                }
            );

            co_return future.get();
        }

        /**
         * @brief Async get a prompt with arguments
         * @param prompt_name Name of the prompt
         * @param arguments Prompt arguments
         * @return Task that resolves to prompt messages
         * @throws std::runtime_error on error
         */
        Task<std::vector<PromptMessage>> get_prompt_async(
            const std::string& prompt_name,
            const std::map<std::string, std::string>& arguments
        ) {
            auto promise = std::make_shared<std::promise<std::vector<PromptMessage>>>();
            auto future = promise->get_future();

            client_.get_prompt(
                prompt_name,
                arguments,
                [promise](const std::vector<PromptMessage>& messages) {
                    promise->set_value(messages);
                },
                [promise](const std::string& error) {
                    promise->set_exception(std::make_exception_ptr(
                        std::runtime_error(error)
                    ));
                }
            );

            co_return future.get();
        }

        /**
         * @brief Async list available resources from server
         * @return Task that resolves to vector of Resource definitions
         * @throws std::runtime_error on error
         */
        Task<std::vector<Resource>> list_resources_async() {
            auto promise = std::make_shared<std::promise<std::vector<Resource>>>();
            auto future = promise->get_future();

            client_.list_resources(
                [promise](const std::vector<Resource>& resources) {
                    promise->set_value(resources);
                },
                [promise](const std::string& error) {
                    promise->set_exception(std::make_exception_ptr(
                        std::runtime_error(error)
                    ));
                }
            );

            co_return future.get();
        }

        /**
         * @brief Async read a resource from server
         * @param uri Resource URI
         * @return Task that resolves to resource contents
         * @throws std::runtime_error on error
         */
        Task<std::vector<ResourceContent>> read_resource_async(
            const std::string& uri
        ) {
            auto promise = std::make_shared<std::promise<std::vector<ResourceContent>>>();
            auto future = promise->get_future();

            client_.read_resource(
                uri,
                [promise](const std::vector<ResourceContent>& contents) {
                    promise->set_value(contents);
                },
                [promise](const std::string& error) {
                    promise->set_exception(std::make_exception_ptr(
                        std::runtime_error(error)
                    ));
                }
            );

            co_return future.get();
        }

        /**
         * @brief Execute multiple tools in parallel
         * @param tool_calls Vector of (tool_name, arguments) pairs
         * @return Task that resolves to vector of results
         * @throws std::runtime_error if any tool fails
         * 
         * @example
         * ```cpp
         * std::vector<std::pair<std::string, json>> calls = {
         *     {"tool1", args1},
         *     {"tool2", args2}
         * };
         * auto results = co_await client.execute_parallel_async(calls);
         * ```
         */
        Task<std::vector<std::vector<ToolResultContent>>> execute_parallel_async(
            const std::vector<std::pair<std::string, json>>& tool_calls
        ) {
            std::vector<std::future<std::vector<ToolResultContent>>> futures;
            futures.reserve(tool_calls.size());

            // Launch all tool calls concurrently
            for (const auto& [tool_name, args] : tool_calls) {
                auto promise = std::make_shared<std::promise<std::vector<ToolResultContent>>>();
                futures.push_back(promise->get_future());

                client_.call_tool(
                    tool_name,
                    args,
                    [promise](const std::vector<ToolResultContent>& result) {
                        promise->set_value(result);
                    },
                    [promise](const std::string& error) {
                        promise->set_exception(std::make_exception_ptr(
                            std::runtime_error(error)
                        ));
                    }
                );
            }

            // Collect all results
            std::vector<std::vector<ToolResultContent>> results;
            results.reserve(futures.size());

            for (auto& future : futures) {
                results.push_back(future.get());
            }

            co_return results;
        }

        /**
         * @brief Get underlying synchronous client
         */
        Client& client() { return client_; }
        const Client& client() const { return client_; }

    private:
        Client& client_;
    };

    /**
     * @brief Helper function to execute async client operation synchronously
     * @param task The async task to execute
     * @return Result of the task
     * 
     * @example
     * ```cpp
     * auto tools = sync_wait(async_client.list_tools_async());
     * ```
     */
    template<typename T>
    T sync_wait_client(Task<T> task) {
        return core::sync_wait(std::move(task));
    }

} // namespace pooriayousefi::mcp
