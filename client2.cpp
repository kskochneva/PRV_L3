#define _WIN32_WINNT 0x0601
#include <boost/asio.hpp>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);
        tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"), 12345);
        socket.connect(endpoint);

        std::cout << "========================================" << std::endl;
        std::cout << "Connected to server!" << std::endl;
        std::cout << "Task 2: Find Maximum" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Enter numbers separated by spaces" << std::endl;
        std::cout << "Example: 10 20 5 88 42" << std::endl;
        std::cout << "Type 'quit' to exit" << std::endl;
        std::cout << "========================================" << std::endl;

        std::string input;
        while (true) {
            std::cout << "\n> ";
            std::getline(std::cin, input);

            if (input == "quit" || input == "exit") {
                break;
            }

            if (input.empty()) {
                continue;
            }

            // Send request with newline
            std::string message = input + "\n";
            boost::asio::write(socket, boost::asio::buffer(message));

            // Read response
            boost::asio::streambuf response_buffer;
            boost::asio::read_until(socket, response_buffer, '\n');

            std::istream response_stream(&response_buffer);
            std::string response;
            std::getline(response_stream, response);

            std::cout << "Server: " << response << std::endl;
        }

        socket.close();

    }
    catch (std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }

    return 0;
}
