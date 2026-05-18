#define _WIN32_WINNT 0x0601
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <algorithm>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, sizeof(data_) - 1),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    data_[length] = '\0';
                    std::string request(data_, length);

                    // Remove newline and carriage return
                    while (!request.empty() && (request.back() == '\n' || request.back() == '\r')) {
                        request.pop_back();
                    }

                    std::cout << "Received: \"" << request << "\"" << std::endl;

                    // Find maximum number
                    std::istringstream iss(request);
                    int num;
                    int maxVal = -2147483648;
                    bool found = false;

                    while (iss >> num) {
                        if (num > maxVal) {
                            maxVal = num;
                        }
                        found = true;
                    }

                    std::string response;
                    if (found) {
                        response = "Maximum: " + std::to_string(maxVal) + "\n";
                        std::cout << "Maximum: " << maxVal << std::endl;
                    }
                    else {
                        response = "Error: send numbers separated by spaces (e.g., 10 20 5 88 42)\n";
                        std::cout << "No numbers found" << std::endl;
                    }

                    do_write(response);
                }
            }
        );
    }

    void do_write(const std::string& response) {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(response),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    do_read();
                }
            }
        );
    }

    tcp::socket socket_;
    char data_[1024];
};

class Server {
public:
    Server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    }

    void start() {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::cout << "\n[New connection!]" << std::endl;
                    std::make_shared<Session>(std::move(socket))->start();
                }
                do_accept();
            }
        );
    }

    tcp::acceptor acceptor_;
};

int main() {
    try {
        boost::asio::io_context io_context;
        Server server(io_context, 12345);
        server.start();

        std::cout << "========================================" << std::endl;
        std::cout << "Server running on port 12345" << std::endl;
        std::cout << "Waiting for connections..." << std::endl;
        std::cout << "========================================" << std::endl;

        io_context.run();

    }
    catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }

    return 0;
}
