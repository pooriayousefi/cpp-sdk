#pragma once

#include "server.hpp"
#include "core/asyncops.hpp"
#include <functional>

/**
 * @file server_streaming.hpp
 * @brief Streaming MCP Server using Generator<T> coroutines
 * 
 * Extends the base Server with Generator<T>-based streaming capabilities
 * for tools, prompts, and resources that produce large or incremental results.
 */

namespace pooriayousefi::mcp 
{
    using core::Generator;

    /**
     * @brief Streaming tool handler that yields results incrementally
     */
    using StreamingToolHandler = std::function<Generator<ToolResultContent>(const json& arguments)>;

    /**
     * @brief Streaming resource reader that yields content chunks
     */
    using StreamingResourceReader = std::function<Generator<ResourceContent>(const std::string& uri)>;

    /**
     * @brief Server extension with streaming capabilities
     * 
     * Adds support for streaming tool handlers and resource readers using Generator<T>.
     * Allows tools and resources to yield results incrementally, preventing memory
     * bloat for large datasets.
     * 
     * @example
     * ```cpp
     * StreamingServer server(transport, impl);
     * 
     * server.register_streaming_tool(tool, [](const json& args) -> Generator<ToolResultContent> {
     *     for (const auto& item : large_dataset) {
     *         co_yield ToolResultContent::text_content(process(item));
     *     }
     * });
     * ```
     */
    class StreamingServer : public Server 
    {
    public:
        using Server::Server; // Inherit constructors

        /**
         * @brief Register a streaming tool that yields results incrementally
         * @param tool Tool definition
         * @param handler Streaming handler that yields ToolResultContent
         * 
         * The handler is a coroutine that yields results one at a time.
         * This prevents loading entire result set into memory.
         * 
         * @example
         * ```cpp
         * Tool search_tool{"search", "Search database", input_schema};
         * 
         * server.register_streaming_tool(search_tool, [](const json& args) {
         *     std::string query = args["query"];
         *     
         *     for (const auto& row : database.search(query)) {
         *         // Yield each result as it's processed
         *         co_yield ToolResultContent::text_content(format(row));
         *         
         *         // Check for cancellation
         *         if (jsonrpc::is_canceled()) {
         *             co_return;
         *         }
         *     }
         * });
         * ```
         */
        void register_streaming_tool(const Tool& tool, StreamingToolHandler handler) {
            // Wrap streaming handler to collect results
            Server::register_tool(tool, [handler = std::move(handler)](const json& args) {
                std::vector<ToolResultContent> results;
                
                // Iterate through generator, collecting results
                for (const auto& content : handler(args)) {
                    results.push_back(content);
                    
                    // Report progress after each chunk
                    if (auto ctx = jsonrpc::current_context()) {
                        jsonrpc::report_progress({
                            {"chunks_processed", results.size()}
                        });
                    }
                    
                    // Check for cancellation
                    if (jsonrpc::is_canceled()) {
                        break;
                    }
                }
                
                return results;
            });
        }

        /**
         * @brief Register a streaming resource that yields content chunks
         * @param resource Resource definition
         * @param reader Streaming reader that yields ResourceContent
         * 
         * @example
         * ```cpp
         * Resource log_resource{"file:///var/log/app.log", "Application Log", ...};
         * 
         * server.register_streaming_resource(log_resource, [](const std::string& uri) {
         *     std::ifstream file(uri_to_path(uri));
         *     std::string line;
         *     
         *     while (std::getline(file, line)) {
         *         co_yield ResourceContent{uri, "text/plain", line + "\n", std::nullopt};
         *     }
         * });
         * ```
         */
        void register_streaming_resource(const Resource& resource, StreamingResourceReader reader) {
            // Wrap streaming reader to collect content
            Server::register_resource(resource, [reader = std::move(reader)](const std::string& uri) {
                std::vector<ResourceContent> contents;
                
                // Iterate through generator, collecting content
                for (const auto& content : reader(uri)) {
                    contents.push_back(content);
                    
                    // Report progress
                    if (auto ctx = jsonrpc::current_context()) {
                        jsonrpc::report_progress({
                            {"bytes_read", content.text ? content.text->size() : 0}
                        });
                    }
                    
                    // Check for cancellation
                    if (jsonrpc::is_canceled()) {
                        break;
                    }
                }
                
                return contents;
            });
        }

        /**
         * @brief Register a tool that streams results with progress reporting
         * @param tool Tool definition
         * @param handler Streaming handler with total count for progress
         * 
         * @example
         * ```cpp
         * server.register_tool_with_progress(
         *     search_tool,
         *     [](const json& args) {
         *         return process_search(args["query"]);
         *     },
         *     [](const json& args) -> size_t {
         *         return estimate_result_count(args["query"]);
         *     }
         * );
         * ```
         */
        void register_tool_with_progress(
            const Tool& tool,
            StreamingToolHandler handler,
            std::function<size_t(const json& args)> estimate_total
        ) {
            Server::register_tool(tool, [
                handler = std::move(handler),
                estimate_total = std::move(estimate_total)
            ](const json& args) {
                size_t total = estimate_total(args);
                size_t processed = 0;
                std::vector<ToolResultContent> results;
                
                for (const auto& content : handler(args)) {
                    results.push_back(content);
                    processed++;
                    
                    // Report progress as percentage
                    if (auto ctx = jsonrpc::current_context()) {
                        double progress = total > 0 ? (double)processed / total : 0.0;
                        jsonrpc::report_progress({
                            {"progress", progress},
                            {"processed", processed},
                            {"total", total}
                        });
                    }
                    
                    if (jsonrpc::is_canceled()) {
                        break;
                    }
                }
                
                return results;
            });
        }
    };

    /**
     * @brief Helper: Stream lines from a file
     * @param path File path
     * @return Generator that yields lines
     * 
     * @example
     * ```cpp
     * for (const auto& line : stream_file_lines("/var/log/app.log")) {
     *     process_line(line);
     * }
     * ```
     */
    inline Generator<std::string> stream_file_lines(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            co_return;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            co_yield line;
        }
    }

    /**
     * @brief Helper: Stream JSON array elements
     * @param json_array JSON array to stream
     * @return Generator that yields individual elements
     * 
     * @example
     * ```cpp
     * json large_array = get_large_array();
     * for (const auto& element : stream_json_array(large_array)) {
     *     process_element(element);
     * }
     * ```
     */
    inline Generator<json> stream_json_array(const json& json_array) {
        if (!json_array.is_array()) {
            co_return;
        }
        
        for (const auto& element : json_array) {
            co_yield element;
        }
    }

    /**
     * @brief Helper: Stream paginated API results
     * @param fetch_page Function that fetches a page (page_num) -> json response
     * @param extract_items Function that extracts items from response
     * @param has_more Function that checks if there are more pages
     * @return Generator that yields all items across all pages
     * 
     * @example
     * ```cpp
     * auto results = stream_paginated_api(
     *     [&](int page) { return api.get("/users?page=" + std::to_string(page)); },
     *     [](const json& resp) { return resp["users"]; },
     *     [](const json& resp) { return resp.value("has_more", false); }
     * );
     * 
     * for (const auto& user : results) {
     *     process_user(user);
     * }
     * ```
     */
    inline Generator<json> stream_paginated_api(
        std::function<json(int page)> fetch_page,
        std::function<json(const json& response)> extract_items,
        std::function<bool(const json& response)> has_more
    ) {
        int page = 0;
        bool continue_fetching = true;
        
        while (continue_fetching) {
            auto response = fetch_page(page);
            auto items = extract_items(response);
            
            if (items.is_array()) {
                for (const auto& item : items) {
                    co_yield item;
                }
            }
            
            continue_fetching = has_more(response);
            page++;
            
            // Check for cancellation between pages
            if (jsonrpc::is_canceled()) {
                co_return;
            }
        }
    }

    /**
     * @brief Helper: Transform generator output
     * @param generator Input generator
     * @param transform Transformation function
     * @return Generator that yields transformed values
     * 
     * @example
     * ```cpp
     * auto lines = stream_file_lines("data.txt");
     * auto uppercase = transform_generator(
     *     std::move(lines),
     *     [](const std::string& line) { return to_uppercase(line); }
     * );
     * ```
     */
    template<typename T, typename U, typename Func>
    Generator<U> transform_generator(Generator<T> generator, Func transform) {
        for (const auto& item : generator) {
            co_yield transform(item);
        }
    }

    /**
     * @brief Helper: Filter generator output
     * @param generator Input generator
     * @param predicate Filter predicate
     * @return Generator that yields only matching values
     * 
     * @example
     * ```cpp
     * auto lines = stream_file_lines("data.txt");
     * auto errors = filter_generator(
     *     std::move(lines),
     *     [](const std::string& line) { return line.find("ERROR") != std::string::npos; }
     * );
     * ```
     */
    template<typename T, typename Pred>
    Generator<T> filter_generator(Generator<T> generator, Pred predicate) {
        for (const auto& item : generator) {
            if (predicate(item)) {
                co_yield item;
            }
        }
    }

} // namespace pooriayousefi::mcp
