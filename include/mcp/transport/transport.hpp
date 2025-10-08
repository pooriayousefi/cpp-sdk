#pragma once

#include "../jsonrpc/jsonrpc.hpp"
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <queue>
#include <mutex>

/**
 * @file transport.hpp
 * @brief Transport layer abstractions for MCP communication
 * 
 * Provides base Transport interface and implementations for:
 * - Standard I/O (stdio) - for command-line tools and subprocess communication
 * - HTTP/SSE - for web-based clients and servers
 * - WebSocket - for bidirectional streaming (future)
 */

namespace pooriayousefi::mcp::transport 
{
    using json = jsonrpc::json;

    /**
     * @brief Base transport interface for sending/receiving JSON-RPC messages
     */
    class Transport 
    {
    public:
        using MessageHandler = std::function<void(const json&)>;
        using ErrorHandler = std::function<void(const std::string&)>;
        using CloseHandler = std::function<void()>;

        virtual ~Transport() = default;

        /**
         * @brief Send a JSON-RPC message
         * @param message JSON-RPC request, response, or notification
         */
        virtual void send(const json& message) = 0;

        /**
         * @brief Start receiving messages (non-blocking)
         */
        virtual void start() = 0;

        /**
         * @brief Stop receiving messages and close transport
         */
        virtual void close() = 0;

        /**
         * @brief Check if transport is connected/running
         */
        virtual bool is_open() const = 0;

        /**
         * @brief Set handler for incoming messages
         */
        void on_message(MessageHandler handler) { 
            message_handler_ = std::move(handler); 
        }

        /**
         * @brief Set handler for transport errors
         */
        void on_error(ErrorHandler handler) { 
            error_handler_ = std::move(handler); 
        }

        /**
         * @brief Set handler for transport close
         */
        void on_close(CloseHandler handler) { 
            close_handler_ = std::move(handler); 
        }

    protected:
        MessageHandler message_handler_;
        ErrorHandler error_handler_;
        CloseHandler close_handler_;

        void emit_message(const json& msg) {
            if (message_handler_) message_handler_(msg);
        }

        void emit_error(const std::string& error) {
            if (error_handler_) error_handler_(error);
        }

        void emit_close() {
            if (close_handler_) close_handler_();
        }
    };

    /**
     * @brief Standard I/O transport (stdin/stdout)
     * 
     * Used for command-line MCP servers that communicate via stdio.
     * Messages are sent as JSON lines (newline-delimited).
     */
    class StdioTransport : public Transport 
    {
    public:
        StdioTransport() : running_(false) {}

        void send(const json& message) override {
            std::lock_guard<std::mutex> lock(write_mutex_);
            try {
                std::cout << message.dump() << "\n" << std::flush;
            } catch (const std::exception& e) {
                emit_error(std::string("Failed to send message: ") + e.what());
            }
        }

        void start() override {
            if (running_) return;
            running_ = true;
            
            // Start reading thread
            read_thread_ = std::thread([this]() {
                std::string line;
                while (running_ && std::getline(std::cin, line)) {
                    if (line.empty()) continue;
                    
                    try {
                        json msg = json::parse(line);
                        emit_message(msg);
                    } catch (const json::exception& e) {
                        emit_error(std::string("JSON parse error: ") + e.what());
                    }
                }
                running_ = false;
                emit_close();
            });
        }

        void close() override {
            if (!running_) return;
            running_ = false;
            if (read_thread_.joinable()) {
                read_thread_.join();
            }
        }

        bool is_open() const override {
            return running_;
        }

        ~StdioTransport() {
            close();
        }

    private:
        std::atomic<bool> running_;
        std::thread read_thread_;
        std::mutex write_mutex_;
    };

    /**
     * @brief In-memory transport for testing
     * 
     * Allows direct message passing without network/stdio overhead.
     * Useful for unit tests and local client-server pairs.
     */
    class InMemoryTransport : public Transport 
    {
    public:
        InMemoryTransport() : running_(false), peer_(nullptr) {}

        /**
         * @brief Connect two in-memory transports as peers
         */
        void connect_peer(std::shared_ptr<InMemoryTransport> peer) {
            peer_ = peer;
            if (peer) {
                peer->peer_ = shared_from_this();
            }
        }

        void send(const json& message) override {
            if (!running_) {
                emit_error("Transport not started");
                return;
            }

            auto peer = peer_.lock();
            if (!peer) {
                emit_error("No peer connected");
                return;
            }

            // Deliver message to peer's queue
            {
                std::lock_guard<std::mutex> lock(peer->queue_mutex_);
                peer->message_queue_.push(message);
            }
            peer->queue_cv_.notify_one();
        }

        void start() override {
            if (running_) return;
            running_ = true;

            // Start message processing thread
            process_thread_ = std::thread([this]() {
                while (running_) {
                    json msg;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        queue_cv_.wait(lock, [this]() { 
                            return !message_queue_.empty() || !running_; 
                        });
                        
                        if (!running_) break;
                        if (message_queue_.empty()) continue;

                        msg = message_queue_.front();
                        message_queue_.pop();
                    }

                    emit_message(msg);
                }
            });
        }

        void close() override {
            if (!running_) return;
            running_ = false;
            queue_cv_.notify_all();
            if (process_thread_.joinable()) {
                process_thread_.join();
            }
            emit_close();
        }

        bool is_open() const override {
            return running_;
        }

        ~InMemoryTransport() {
            close();
        }

    private:
        std::atomic<bool> running_;
        std::weak_ptr<InMemoryTransport> peer_;
        std::queue<json> message_queue_;
        std::mutex queue_mutex_;
        std::condition_variable queue_cv_;
        std::thread process_thread_;
    };

    /**
     * @brief Create a pair of connected in-memory transports
     * @return std::pair of connected transports (client, server)
     */
    inline std::pair<std::shared_ptr<InMemoryTransport>, std::shared_ptr<InMemoryTransport>>
    create_in_memory_pair() {
        auto client = std::make_shared<InMemoryTransport>();
        auto server = std::make_shared<InMemoryTransport>();
        client->connect_peer(server);
        return {client, server};
    }

} // namespace pooriayousefi::mcp::transport
