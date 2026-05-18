#define _WIN32_WINNT 0x0601
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <sstream>
#include <mutex>

using boost::asio::ip::tcp;

// Global counter for connections (shared resource)
int g_connection_counter = 0;
std::vector<std::string> g_log;  // shared log storage

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket,
        boost::asio::strand<boost::asio::io_context::executor_type> strand)
        : socket_(std::move(socket)), strand_(strand) {
    }

    void start() {
        // Increment connection counter safely via strand
        boost::asio::post(strand_, [this]() {
            ++g_connection_counter;
            log("Connection accepted. Total connections: " + std::to_string(g_connection_counter));
            });
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, sizeof(data_) - 1),
            boost::asio::bind_executor(strand_,
                [this, self](boost::system::error_code ec, std::size_t length) {
                    if (!ec) {
                        data_[length] = '\0';
                        std::string request(data_, length);
                        // Remove newline/carriage return
                        while (!request.empty() && (request.back() == '\n' || request.back() == '\r'))
                            request.pop_back();

                        log("Received: " + request);
                        // Process request asynchronously (heavy computation)
                        processRequestAsync(request);
                    }
                    else {
                        log("Read error: " + ec.message());
                        // Decrement counter on disconnect
                        boost::asio::post(strand_, [this]() {
                            --g_connection_counter;
                            log("Connection closed. Remaining: " + std::to_string(g_connection_counter));
                            });
                    }
                }
            )
        );
    }

    void processRequestAsync(const std::string& request) {
        auto self(shared_from_this());
        // Post heavy computation to the strand to avoid blocking the thread pool
        boost::asio::post(strand_, [this, self, request]() {
            std::string response;
            // Try to parse as a number for factorial
            try {
                int n = std::stoi(request);
                if (n < 0) {
                    response = "Error: factorial of negative number is undefined\n";
                }
                else if (n > 20) {
                    response = "Error: number too large (max 20 for 64-bit result)\n";
                }
                else {
                    // Simulate heavy computation (e.g., factorial with delay)
                    unsigned long long result = 1;
                    for (int i = 2; i <= n; ++i) {
                        result *= i;
                        // Artificial delay to simulate heavy work (0.1 sec per multiply)
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    response = "Factorial(" + std::to_string(n) + ") = " + std::to_string(result) + "\n";
                    log("Computed factorial(" + std::to_string(n) + ") = " + std::to_string(result));
                }
            }
            catch (...) {
                // If not a number, treat as in Task 2 (find maximum)
                std::istringstream iss(request);
                int num, maxVal = -2147483648;
                bool found = false;
                while (iss >> num) {
                    if (num > maxVal) maxVal = num;
                    found = true;
                }
                if (found)
                    response = "Maximum: " + std::to_string(maxVal) + "\n";
                else
                    response = "Error: send a number (e.g., 5) for factorial or numbers for max\n";
            }
            do_write(response);
            });
    }

    void do_write(const std::string& response) {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(response),
            boost::asio::bind_executor(strand_,
                [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                    if (!ec) {
                        do_read();  // continue reading next request
                    }
                    else {
                        log("Write error: " + ec.message());
                        // Decrement counter on disconnect
                        boost::asio::post(strand_, [this]() {
                            --g_connection_counter;
                            log("Connection closed. Remaining: " + std::to_string(g_connection_counter));
                            });
                    }
                }
            )
        );
    }

    void log(const std::string& msg) {
        // This is always called from within the strand, so safe
        g_log.push_back(msg);
        std::cout << "[LOG] " << msg << std::endl;
    }

    tcp::socket socket_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    char data_[1024];
};

class Server {
public:
    Server(boost::asio::io_context& io_context, short port, size_t thread_pool_size)
        : io_context_(io_context),
        acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
        strand_(io_context.get_executor()),
        thread_pool_size_(thread_pool_size) {
    }

    void start() {
        do_accept();
        // Log server start
        boost::asio::post(strand_, []() {
            g_log.push_back("Server started with " + std::to_string(std::thread::hardware_concurrency()) + " hardware threads");
            });
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            boost::asio::bind_executor(strand_,
                [this](boost::system::error_code ec, tcp::socket socket) {
                    if (!ec) {
                        std::make_shared<Session>(std::move(socket), strand_)->start();
                    }
                    do_accept();
                }
            )
        );
    }

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    size_t thread_pool_size_;
};

int main(int argc, char* argv[]) {
    try {
        short port = 12345;
        size_t thread_count = 4;  // default thread pool size

        if (argc > 1) port = static_cast<short>(std::stoi(argv[1]));
        if (argc > 2) thread_count = static_cast<size_t>(std::stoi(argv[2]));

        boost::asio::io_context io_context(thread_count);  // concurrency hint

        Server server(io_context, port, thread_count);
        server.start();

        std::cout << "========================================" << std::endl;
        std::cout << "Multithreaded Server (Task 4)" << std::endl;
        std::cout << "Port: " << port << ", Thread pool size: " << thread_count << std::endl;
        std::cout << "Commands: send a number (e.g., 5) for factorial," << std::endl;
        std::cout << "          or numbers separated by spaces for max." << std::endl;
        std::cout << "========================================" << std::endl;

        // Create thread pool
        std::vector<std::thread> threads;
        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([&io_context]() {
                io_context.run();
                });
        }

        // Wait for all threads (in this example, server runs forever, so we wait)
        for (auto& t : threads) {
            t.join();
        }

    }
    catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }

    return 0;
}
