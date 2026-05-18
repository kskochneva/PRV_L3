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
        std::cout << "Task 3: Async Timer" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  timer N   - server waits N seconds then sends 'Done!'" << std::endl;
        std::cout << "  numbers   - server returns maximum" << std::endl;
        std::cout << "  quit      - exit" << std::endl;
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

            // Send request
            std::string message = input + "\n";
            boost::asio::write(socket, boost::asio::buffer(message));

            // Read response (may be multiple lines for timer)
            while (true) {
                boost::asio::streambuf response_buffer;
                boost::asio::read_until(socket, response_buffer, '\n');

                std::istream response_stream(&response_buffer);
                std::string response;
                std::getline(response_stream, response);

                std::cout << "Server: " << response << std::endl;

                // For timer command, we receive "Ready in X sec" immediately,
                // then "Done!" arrives later through the same connection.
                // Check if we need to wait for more.
                if (response.find("Ready in") != std::string::npos) {
                    // The server will send "Done!" after the timer expires.
                    // Continue reading for the next message.
                    continue;
                }
                break;
            }
        }

        socket.close();

    }
    catch (std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }

    return 0;
}
