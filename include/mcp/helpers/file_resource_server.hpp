#pragma once

#include "server.hpp"
#include "server_streaming.hpp"
#include "core/raiiiofsw.hpp"
#include "core/asyncops.hpp"
#include <filesystem>
#include <unordered_map>
#include <mutex>

/**
 * @file helpers/file_resource_server.hpp
 * @brief File-based resource server with RAII file I/O
 * 
 * Provides automatic serving of files as MCP resources using
 * RAII file wrappers for safe, leak-free file operations.
 */

namespace pooriayousefi::mcp::helpers 
{
    namespace fs = std::filesystem;
    using core::Generator;

    /**
     * @brief Detect MIME type from file extension
     */
    inline std::string detect_mime_type(const std::string& path) {
        static const std::unordered_map<std::string, std::string> mime_types = {
            {".txt", "text/plain"},
            {".md", "text/markdown"},
            {".html", "text/html"},
            {".css", "text/css"},
            {".js", "application/javascript"},
            {".json", "application/json"},
            {".xml", "application/xml"},
            {".pdf", "application/pdf"},
            {".png", "image/png"},
            {".jpg", "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".gif", "image/gif"},
            {".svg", "image/svg+xml"},
            {".zip", "application/zip"},
            {".tar", "application/x-tar"},
            {".gz", "application/gzip"}
        };

        fs::path file_path(path);
        std::string extension = file_path.extension().string();
        
        auto it = mime_types.find(extension);
        if (it != mime_types.end()) {
            return it->second;
        }
        
        return "application/octet-stream";
    }

    /**
     * @brief Parse file:// URI to filesystem path
     */
    inline std::string parse_file_uri(const std::string& uri) {
        if (uri.substr(0, 7) == "file://") {
            return uri.substr(7);
        }
        return uri;
    }

    /**
     * @brief File resource server with RAII file operations
     * 
     * Serves files from a directory tree as MCP resources using
     * RAII file wrappers for automatic cleanup and error handling.
     * 
     * Features:
     * - Automatic MIME type detection
     * - Safe file reading with RAII
     * - Directory listing
     * - Streaming for large files
     * - Path traversal protection
     * 
     * @example
     * ```cpp
     * Server server(transport, impl);
     * FileResourceServer file_server(server, "/home/user/documents");
     * file_server.enable_streaming(true);  // Stream large files
     * file_server.set_max_file_size(10 * 1024 * 1024);  // 10MB limit
     * ```
     */
    class FileResourceServer {
    public:
        /**
         * @brief Construct file resource server
         * @param server MCP server to register resources with
         * @param root_directory Root directory to serve files from
         * @param url_prefix URI prefix (default: "file://")
         */
        FileResourceServer(
            Server& server,
            const std::string& root_directory,
            const std::string& url_prefix = "file://"
        )
            : server_(server)
            , root_dir_(fs::absolute(root_directory))
            , url_prefix_(url_prefix)
            , max_file_size_(50 * 1024 * 1024) // 50MB default
            , enable_streaming_(false)
        {
            if (!fs::exists(root_dir_) || !fs::is_directory(root_dir_)) {
                throw std::runtime_error("Root directory does not exist: " + root_directory);
            }

            register_resources();
        }

        /**
         * @brief Set maximum file size to serve (bytes)
         */
        void set_max_file_size(size_t max_bytes) {
            max_file_size_ = max_bytes;
        }

        /**
         * @brief Enable/disable streaming for large files
         */
        void enable_streaming(bool enable) {
            enable_streaming_ = enable;
        }

        /**
         * @brief List all files in root directory
         * @return Vector of Resource definitions
         */
        std::vector<Resource> list_files() {
            std::vector<Resource> resources;
            
            for (const auto& entry : fs::recursive_directory_iterator(root_dir_)) {
                if (entry.is_regular_file()) {
                    std::string rel_path = fs::relative(entry.path(), root_dir_).string();
                    std::string uri = url_prefix_ + rel_path;
                    
                    resources.push_back(Resource{
                        uri,
                        entry.path().filename().string(),
                        "File: " + rel_path,
                        detect_mime_type(entry.path().string())
                    });
                }
            }
            
            return resources;
        }

        /**
         * @brief Refresh resource list (call after files change)
         */
        void refresh() {
            std::lock_guard<std::mutex> lock(mutex_);
            cached_resources_ = list_files();
        }

    private:
        void register_resources() {
            // Cache initial file list
            refresh();

            // Register resources/list handler
            server_.add("resources/list", [this](const json&) -> json {
                std::lock_guard<std::mutex> lock(mutex_);
                
                json resources_array = json::array();
                for (const auto& resource : cached_resources_) {
                    resources_array.push_back(resource.to_json());
                }
                
                return json{{"resources", resources_array}};
            });

            // Register resources/read handler
            server_.add("resources/read", [this](const json& params) -> json {
                std::string uri = params.value("uri", "");
                if (uri.empty()) {
                    throw jsonrpc::rpc_exception(jsonrpc::error{
                        -32602, "Missing resource URI", nullptr
                    });
                }

                auto contents = read_file_resource(uri);
                
                json contents_array = json::array();
                for (const auto& content : contents) {
                    contents_array.push_back(content.to_json());
                }
                
                return json{{"contents", contents_array}};
            });
        }

        std::vector<ResourceContent> read_file_resource(const std::string& uri) {
            // Parse URI to path
            std::string rel_path = parse_file_uri(uri);
            if (rel_path.substr(0, url_prefix_.size()) == url_prefix_) {
                rel_path = rel_path.substr(url_prefix_.size());
            }

            // Resolve to absolute path
            fs::path abs_path = root_dir_ / rel_path;
            abs_path = fs::absolute(abs_path);

            // Security: Ensure path is within root directory (prevent traversal)
            std::string abs_path_str = abs_path.string();
            std::string root_str = root_dir_.string();
            if (abs_path_str.substr(0, root_str.size()) != root_str) {
                throw jsonrpc::rpc_exception(jsonrpc::error{
                    -32602, "Path traversal not allowed", nullptr
                });
            }

            // Check file exists
            if (!fs::exists(abs_path) || !fs::is_regular_file(abs_path)) {
                throw jsonrpc::rpc_exception(jsonrpc::error{
                    -32601, "File not found: " + rel_path, nullptr
                });
            }

            // Check file size
            size_t file_size = fs::file_size(abs_path);
            if (file_size > max_file_size_) {
                throw jsonrpc::rpc_exception(jsonrpc::error{
                    -32603, "File too large (max " + std::to_string(max_file_size_) + " bytes)", nullptr
                });
            }

            // Use RAII file wrapper for safe reading
            core::RAIIInputFileStream file(abs_path.string());
            
            if (!file.is_open()) {
                throw jsonrpc::rpc_exception(jsonrpc::error{
                    -32603, "Failed to open file: " + rel_path, nullptr
                });
            }

            // Read file content
            std::string content;
            
            if (enable_streaming_ && file_size > 1024 * 1024) { // 1MB threshold
                // Stream large files in chunks
                const size_t chunk_size = 64 * 1024; // 64KB chunks
                content.reserve(file_size);
                
                char buffer[chunk_size];
                while (file.stream().read(buffer, chunk_size) || file.stream().gcount() > 0) {
                    content.append(buffer, file.stream().gcount());
                    
                    // Report progress
                    if (auto ctx = jsonrpc::current_context()) {
                        double progress = (double)content.size() / file_size;
                        jsonrpc::report_progress({
                            {"progress", progress},
                            {"bytes_read", content.size()},
                            {"total_bytes", file_size}
                        });
                    }
                    
                    // Check cancellation
                    if (jsonrpc::is_canceled()) {
                        throw jsonrpc::rpc_exception(jsonrpc::error{
                            -32800, "Read cancelled", nullptr
                        });
                    }
                }
            } else {
                // Read small files at once
                content = file.read_all();
            }

            // File automatically closed by RAII wrapper

            std::string mime_type = detect_mime_type(abs_path.string());

            return {ResourceContent{
                uri,
                mime_type,
                content,
                std::nullopt // blob (for binary files in future)
            }};
        }

        Server& server_;
        fs::path root_dir_;
        std::string url_prefix_;
        size_t max_file_size_;
        bool enable_streaming_;
        std::vector<Resource> cached_resources_;
        std::mutex mutex_;
    };

    /**
     * @brief Streaming file resource server
     * 
     * Extended version using StreamingServer for true streaming of large files
     * without loading entire file into memory.
     * 
     * @example
     * ```cpp
     * StreamingServer server(transport, impl);
     * StreamingFileResourceServer file_server(server, "/var/log");
     * // Logs can be huge - stream them!
     * ```
     */
    class StreamingFileResourceServer {
    public:
        StreamingFileResourceServer(
            StreamingServer& server,
            const std::string& root_directory,
            const std::string& url_prefix = "file://"
        )
            : server_(server)
            , root_dir_(fs::absolute(root_directory))
            , url_prefix_(url_prefix)
        {
            if (!fs::exists(root_dir_) || !fs::is_directory(root_dir_)) {
                throw std::runtime_error("Root directory does not exist: " + root_directory);
            }

            register_streaming_resources();
        }

    private:
        void register_streaming_resources() {
            // Register each file as a streaming resource
            for (const auto& entry : fs::recursive_directory_iterator(root_dir_)) {
                if (entry.is_regular_file()) {
                    std::string rel_path = fs::relative(entry.path(), root_dir_).string();
                    std::string uri = url_prefix_ + rel_path;
                    
                    Resource resource{
                        uri,
                        entry.path().filename().string(),
                        "File: " + rel_path,
                        detect_mime_type(entry.path().string())
                    };

                    // Register with streaming reader
                    server_.register_streaming_resource(
                        resource,
                        [this](const std::string& uri) {
                            return stream_file_content(uri);
                        }
                    );
                }
            }
        }

        Generator<ResourceContent> stream_file_content(const std::string& uri) {
            std::string rel_path = parse_file_uri(uri);
            if (rel_path.substr(0, url_prefix_.size()) == url_prefix_) {
                rel_path = rel_path.substr(url_prefix_.size());
            }

            fs::path abs_path = root_dir_ / rel_path;
            abs_path = fs::absolute(abs_path);

            // Security check
            if (abs_path.string().substr(0, root_dir_.string().size()) != root_dir_.string()) {
                co_return; // Path traversal attempt
            }

            // Use RAII file wrapper
            core::RAIIInputFileStream file(abs_path.string());
            
            if (!file.is_open()) {
                co_return;
            }

            // Stream file line-by-line
            std::string line;
            std::string mime_type = detect_mime_type(abs_path.string());
            
            while (std::getline(file.stream(), line)) {
                co_yield ResourceContent{
                    uri,
                    mime_type,
                    line + "\n",
                    std::nullopt
                };
                
                // Check for cancellation
                if (jsonrpc::is_canceled()) {
                    co_return;
                }
            }

            // File automatically closed by RAII wrapper
        }

        StreamingServer& server_;
        fs::path root_dir_;
        std::string url_prefix_;
    };

} // namespace pooriayousefi::mcp::helpers
