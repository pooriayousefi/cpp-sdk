#pragma once

#include "transport.hpp"
#include "../core/asyncops.hpp"
#include <httplib.h>
#include <sstream>
#include <thread>
#include <atomic>
#include <condition_variable>

/**
 * @file transport/http_transport.hpp
 * @brief HTTP/SSE transport with async operations
 * 
 * Provides HTTP-based MCP transport using Task<T> for async requests
 * and Generator<T> for Server-Sent Events streaming.
 * 
 * @note Requires cpp-httplib: https://github.com/yhirose/cpp-httplib
 *       Header-only library, include in your project or install via package manager
 */

namespace pooriayousefi::mcp::transport 
{
    using core::Task;
    using core::Generator;

    /**
     * @brief HTTP client transport with async operations
     * 
     * Sends JSON-RPC messages over HTTP POST and receives responses.
     * Uses Task<T> for non-blocking async operations.
     * 
     * @example
     * ```cpp
     * HttpClientTransport transport("http://localhost:8080");
     * 
     * // Async send/receive
     * auto task = transport.send_async(request_json);
     * json response = sync_wait(task);
     * ```
     */
    class HttpClientTransport : public Transport {
    public:
        /**
         * @brief Construct HTTP client
         * @param base_url Base URL of MCP server (e.g., "http://localhost:8080")
         * @param endpoint JSON-RPC endpoint path (default: "/jsonrpc")
         */
        HttpClientTransport(
            const std::string& base_url,
            const std::string& endpoint = "/jsonrpc"
        )
            : base_url_(base_url)
            , endpoint_(endpoint)
            , client_(parse_host(base_url), parse_port(base_url))
        {
            client_.set_connection_timeout(5, 0); // 5 second timeout
            client_.set_read_timeout(30, 0);      // 30 second read timeout
            client_.set_write_timeout(30, 0);     // 30 second write timeout
        }

        void send(const json& message) override {
            std::string body = message.dump();
            
            auto res = client_.Post(endpoint_.c_str(), body, "application/json");
            
            if (!res) {
                throw std::runtime_error("HTTP request failed: " + httplib::to_string(res.error()));
            }
            
            if (res->status != 200) {
                throw std::runtime_error("HTTP error: " + std::to_string(res->status));
            }

            // Store response for receive()
            std::lock_guard<std::mutex> lock(response_mutex_);
            last_response_ = json::parse(res->body);
            response_ready_.notify_one();
        }

        json receive() override {
            std::unique_lock<std::mutex> lock(response_mutex_);
            response_ready_.wait(lock, [this] { return !last_response_.is_null(); });
            
            json response = std::move(last_response_);
            last_response_ = json{};
            return response;
        }

        /**
         * @brief Async send and receive
         * @param message JSON-RPC message to send
         * @return Task that resolves to response JSON
         */
        Task<json> send_async(const json& message) {
            std::string body = message.dump();
            
            // Launch async HTTP request
            co_return co_await std::async(std::launch::async, [this, body]() -> json {
                auto res = client_.Post(endpoint_.c_str(), body, "application/json");
                
                if (!res) {
                    throw std::runtime_error("HTTP request failed: " + httplib::to_string(res.error()));
                }
                
                if (res->status != 200) {
                    throw std::runtime_error("HTTP error: " + std::to_string(res->status));
                }

                return json::parse(res->body);
            });
        }

        /**
         * @brief Set request timeout
         * @param seconds Timeout in seconds
         */
        void set_timeout(int seconds) {
            client_.set_connection_timeout(seconds, 0);
            client_.set_read_timeout(seconds, 0);
            client_.set_write_timeout(seconds, 0);
        }

        /**
         * @brief Set custom headers
         * @param headers Map of header name to value
         */
        void set_headers(const std::map<std::string, std::string>& headers) {
            headers_ = headers;
            for (const auto& [key, value] : headers_) {
                client_.set_default_headers({{key.c_str(), value.c_str()}});
            }
        }

    private:
        static std::string parse_host(const std::string& url) {
            // Parse http://host:port -> host
            size_t start = url.find("://");
            if (start == std::string::npos) return "localhost";
            start += 3;
            
            size_t end = url.find(':', start);
            if (end == std::string::npos) end = url.find('/', start);
            if (end == std::string::npos) end = url.size();
            
            return url.substr(start, end - start);
        }

        static int parse_port(const std::string& url) {
            // Parse http://host:port -> port
            size_t start = url.find("://");
            if (start == std::string::npos) return 80;
            start += 3;
            
            size_t colon = url.find(':', start);
            if (colon == std::string::npos) {
                return url.substr(0, 5) == "https" ? 443 : 80;
            }
            
            size_t end = url.find('/', colon);
            if (end == std::string::npos) end = url.size();
            
            return std::stoi(url.substr(colon + 1, end - colon - 1));
        }

        std::string base_url_;
        std::string endpoint_;
        httplib::Client client_;
        std::map<std::string, std::string> headers_;
        
        json last_response_;
        std::mutex response_mutex_;
        std::condition_variable response_ready_;
    };

    /**
     * @brief Server-Sent Events (SSE) client transport
     * 
     * Receives streaming JSON-RPC messages via SSE using Generator<T>
     * for incremental processing.
     * 
     * @example
     * ```cpp
     * SseClientTransport sse("http://localhost:8080/events");
     * 
     * for (auto message : sse.receive_stream()) {
     *     // Process each streaming message
     *     handle_message(message);
     * }
     * ```
     */
    class SseClientTransport {
    public:
        /**
         * @brief Construct SSE client
         * @param url SSE endpoint URL
         */
        SseClientTransport(const std::string& url)
            : url_(url)
            , running_(false)
        {}

        ~SseClientTransport() {
            stop();
        }

        /**
         * @brief Start SSE connection
         */
        void start() {
            if (running_.exchange(true)) {
                return; // Already running
            }

            receiver_thread_ = std::thread([this] {
                receive_loop();
            });
        }

        /**
         * @brief Stop SSE connection
         */
        void stop() {
            running_ = false;
            if (receiver_thread_.joinable()) {
                receiver_thread_.join();
            }
        }

        /**
         * @brief Receive messages as generator stream
         * @return Generator yielding each SSE message
         */
        Generator<json> receive_stream() {
            while (running_) {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                
                queue_cv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
                    return !message_queue_.empty() || !running_;
                });

                if (!message_queue_.empty()) {
                    json message = std::move(message_queue_.front());
                    message_queue_.pop();
                    co_yield message;
                } else if (!running_) {
                    co_return;
                }
            }
        }

    private:
        void receive_loop() {
            httplib::Client client(parse_host(url_), parse_port(url_));
            
            client.Get(parse_path(url_).c_str(), {}, 
                [this](const httplib::Response& response) {
                    return running_;
                },
                [this](const char* data, size_t length) {
                    std::string chunk(data, length);
                    process_sse_chunk(chunk);
                    return running_;
                }
            );
        }

        void process_sse_chunk(const std::string& chunk) {
            // Parse SSE format: "data: {...}\n\n"
            std::istringstream stream(chunk);
            std::string line;
            std::string data;

            while (std::getline(stream, line)) {
                if (line.substr(0, 6) == "data: ") {
                    data = line.substr(6);
                } else if (line.empty() && !data.empty()) {
                    // Complete message
                    try {
                        json message = json::parse(data);
                        
                        std::lock_guard<std::mutex> lock(queue_mutex_);
                        message_queue_.push(message);
                        queue_cv_.notify_one();
                    } catch (...) {
                        // Invalid JSON, skip
                    }
                    data.clear();
                }
            }
        }

        static std::string parse_host(const std::string& url) {
            size_t start = url.find("://");
            if (start == std::string::npos) return "localhost";
            start += 3;
            
            size_t end = url.find(':', start);
            if (end == std::string::npos) end = url.find('/', start);
            if (end == std::string::npos) end = url.size();
            
            return url.substr(start, end - start);
        }

        static int parse_port(const std::string& url) {
            size_t start = url.find("://");
            if (start == std::string::npos) return 80;
            start += 3;
            
            size_t colon = url.find(':', start);
            if (colon == std::string::npos) {
                return url.substr(0, 5) == "https" ? 443 : 80;
            }
            
            size_t end = url.find('/', colon);
            if (end == std::string::npos) end = url.size();
            
            return std::stoi(url.substr(colon + 1, end - colon - 1));
        }

        static std::string parse_path(const std::string& url) {
            size_t start = url.find("://");
            if (start == std::string::npos) return "/";
            start += 3;
            
            size_t path_start = url.find('/', start);
            if (path_start == std::string::npos) return "/";
            
            return url.substr(path_start);
        }

        std::string url_;
        std::atomic<bool> running_;
        std::thread receiver_thread_;
        std::queue<json> message_queue_;
        std::mutex queue_mutex_;
        std::condition_variable queue_cv_;
    };

    /**
     * @brief HTTP server transport for hosting MCP server
     * 
     * Listens for JSON-RPC requests over HTTP POST and sends responses.
     * Supports SSE for streaming notifications.
     * 
     * @example
     * ```cpp
     * HttpServerTransport transport(8080);
     * Server server(transport, impl);
     * 
     * transport.start(); // Start HTTP server
     * // Server now listening on http://localhost:8080
     * ```
     */
    class HttpServerTransport : public Transport {
    public:
        /**
         * @brief Construct HTTP server
         * @param port Port to listen on
         * @param host Host to bind to (default: "0.0.0.0" for all interfaces)
         */
        HttpServerTransport(int port, const std::string& host = "0.0.0.0")
            : port_(port)
            , host_(host)
            , server_(std::make_unique<httplib::Server>())
            , running_(false)
        {
            setup_routes();
        }

        ~HttpServerTransport() {
            stop();
        }

        void send(const json& message) override {
            // Store response for next HTTP response
            std::lock_guard<std::mutex> lock(response_mutex_);
            pending_response_ = message;
            response_ready_.notify_one();
        }

        json receive() override {
            std::unique_lock<std::mutex> lock(request_mutex_);
            request_ready_.wait(lock, [this] { return !pending_request_.is_null(); });
            
            json request = std::move(pending_request_);
            pending_request_ = json{};
            return request;
        }

        /**
         * @brief Start HTTP server
         */
        void start() {
            if (running_.exchange(true)) {
                return;
            }

            server_thread_ = std::thread([this] {
                server_->listen(host_.c_str(), port_);
            });
        }

        /**
         * @brief Stop HTTP server
         */
        void stop() {
            if (running_.exchange(false)) {
                server_->stop();
                if (server_thread_.joinable()) {
                    server_thread_.join();
                }
            }
        }

        /**
         * @brief Send SSE notification to all connected clients
         * @param notification Notification message
         */
        void send_sse_notification(const json& notification) {
            std::lock_guard<std::mutex> lock(sse_mutex_);
            
            std::string data = "data: " + notification.dump() + "\n\n";
            
            for (auto& sink : sse_sinks_) {
                if (sink) {
                    sink->write(data.c_str(), data.size());
                }
            }
        }

    private:
        void setup_routes() {
            // JSON-RPC endpoint
            server_->Post("/jsonrpc", [this](const httplib::Request& req, httplib::Response& res) {
                handle_jsonrpc_request(req, res);
            });

            // SSE endpoint
            server_->Get("/events", [this](const httplib::Request& req, httplib::Response& res) {
                handle_sse_connection(req, res);
            });

            // Health check
            server_->Get("/health", [](const httplib::Request&, httplib::Response& res) {
                res.set_content("{\"status\":\"ok\"}", "application/json");
            });
        }

        void handle_jsonrpc_request(const httplib::Request& req, httplib::Response& res) {
            try {
                json request = json::parse(req.body);
                
                // Queue request
                {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    pending_request_ = request;
                    request_ready_.notify_one();
                }

                // Wait for response
                std::unique_lock<std::mutex> lock(response_mutex_);
                response_ready_.wait_for(lock, std::chrono::seconds(30), [this] {
                    return !pending_response_.is_null();
                });

                if (!pending_response_.is_null()) {
                    res.set_content(pending_response_.dump(), "application/json");
                    pending_response_ = json{};
                } else {
                    res.status = 504; // Gateway timeout
                    res.set_content("{\"error\":\"Timeout\"}", "application/json");
                }
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(
                    json{{"error", e.what()}}.dump(),
                    "application/json"
                );
            }
        }

        void handle_sse_connection(const httplib::Request&, httplib::Response& res) {
            res.set_header("Content-Type", "text/event-stream");
            res.set_header("Cache-Control", "no-cache");
            res.set_header("Connection", "keep-alive");

            // Register SSE sink
            {
                std::lock_guard<std::mutex> lock(sse_mutex_);
                sse_sinks_.push_back(&res);
            }

            // Keep connection alive
            res.set_content_provider(
                "text/event-stream",
                [this](size_t, httplib::DataSink& sink) {
                    // Keep alive ping every 30 seconds
                    std::this_thread::sleep_for(std::chrono::seconds(30));
                    sink.write(": ping\n\n", 8);
                    return running_;
                }
            );

            // Remove sink when connection closes
            {
                std::lock_guard<std::mutex> lock(sse_mutex_);
                sse_sinks_.erase(
                    std::remove(sse_sinks_.begin(), sse_sinks_.end(), &res),
                    sse_sinks_.end()
                );
            }
        }

        int port_;
        std::string host_;
        std::unique_ptr<httplib::Server> server_;
        std::atomic<bool> running_;
        std::thread server_thread_;

        json pending_request_;
        json pending_response_;
        std::mutex request_mutex_;
        std::mutex response_mutex_;
        std::condition_variable request_ready_;
        std::condition_variable response_ready_;

        std::vector<httplib::Response*> sse_sinks_;
        std::mutex sse_mutex_;
    };

} // namespace pooriayousefi::mcp::transport
