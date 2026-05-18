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
        std::cout << "Connected to multithreaded server (Task 4)" << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  <number>         -> compute factorial (e.g., 5)" << std::endl;
        std::cout << "  <nums>           -> find maximum (e.g., 10 20 5 88)" << std::endl;
        std::cout << "  quit             -> exit" << std::endl;
        std::cout << "========================================" << std::endl;

        std::string input;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, input);
            if (input == "quit" || input == "exit") break;
            if (input.empty()) continue;

            boost::asio::write(socket, boost::asio::buffer(input + "\n"));

            boost::asio::streambuf response_buffer;
            boost::asio::read_until(socket, response_buffer, '\n');

            std::istream is(&response_buffer);
            std::string response;
            std::getline(is, response);
            std::cout << "Server: " << response << std::endl;
        }
        socket.close();
    }
    catch (std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
    return 0;
}
