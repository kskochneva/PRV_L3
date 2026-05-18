#define _WIN32_WINNT 0x0601
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <chrono>

using boost::asio::ip::tcp;
using namespace std::chrono_literals;

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

                    // Check if command is "timer N"
                    if (request.substr(0, 5) == "timer" || request.substr(0, 5) == "Timer") {
                        handleTimerCommand(request);
                    }
                    // Also keep Task 2 functionality (find maximum)
                    else {
                        handleNumbers(request);
                    }
                }
            }
        );
    }

    // Задача 3: Обработка команды timer
    void handleTimerCommand(const std::string& request) {
        // Extract number after "timer"
        std::string numPart = request.substr(5);

        // Remove leading spaces
        size_t start = numPart.find_first_not_of(" \t");
        if (start != std::string::npos) {
            numPart = numPart.substr(start);
        }

        int seconds = 0;
        try {
            seconds = std::stoi(numPart);
        }
        catch (...) {
            std::string error = "Error: invalid timer command. Use: timer <seconds>\n";
            do_write(error);
            return;
        }

        if (seconds <= 0) {
            std::string error = "Error: seconds must be positive\n";
            do_write(error);
            return;
        }

        // Send immediate response
        std::string response = "Ready in " + std::to_string(seconds) + " sec\n";
        std::cout << "Timer set for " << seconds << " seconds" << std::endl;
        do_write(response);

        // Start async timer
        startTimer(seconds);
    }

    void startTimer(int seconds) {
        auto self(shared_from_this());

        // Create timer
        auto timer = std::make_shared<boost::asio::steady_timer>(socket_.get_executor());
        timer->expires_after(std::chrono::seconds(seconds));

        // Async wait
        timer->async_wait([this, self, timer](boost::system::error_code ec) {
            if (!ec) {
                std::string done = "Done!\n";
                std::cout << "Timer finished! Sending 'Done!'" << std::endl;

                // Send "Done!" to client
                boost::asio::async_write(
                    socket_,
                    boost::asio::buffer(done),
                    [this, self](boost::system::error_code ec, std::size_t) {
                        if (!ec) {
                            // After sending Done!, continue listening
                            do_read();
                        }
                    }
                );
            }
            else {
                std::cout << "Timer error: " << ec.message() << std::endl;
            }
            });
    }

    // Задача 2: Поиск максимума (оставляем для совместимости)
    void handleNumbers(const std::string& request) {
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
            response = "Error: send numbers (e.g., 10 20 5) or 'timer N'\n";
            std::cout << "No numbers found" << std::endl;
        }

        do_write(response);
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
        std::cout << "Task 3: Async Timer" << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  timer N   - wait N seconds, then send 'Done!'" << std::endl;
        std::cout << "  numbers   - find maximum (Task 2)" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Waiting for connections..." << std::endl;

        io_context.run();

    }
    catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }

    return 0;
}
