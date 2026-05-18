#include <server.hpp>
#include <iostream>
#include <algorithm>
#include <sstream>

using namespace std;

SyncServer::SyncServer(boost::asio::io_context& io, short port)
    : acceptor_(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
}

std::string SyncServer::process(const std::string& input) {
    std::string upper = input;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    std::stringstream ss;
    ss << upper.size() << ": " << upper;
    return ss.str();
}

void SyncServer::run() {
    std::cout << "Sync server running on port " << acceptor_.local_endpoint().port() << "\n";

    while (true) {
        boost::asio::ip::tcp::socket socket(acceptor_.get_executor());
        acceptor_.accept(socket);

        boost::asio::streambuf buf;
        boost::asio::read_until(socket, buf, '\n');

        std::istream is(&buf);
        std::string line;
        std::getline(is, line);

        std::string response = process(line) + "\n";
        boost::asio::write(socket, boost::asio::buffer(response));
    }
}