#pragma once

#include <boost/asio.hpp>
#include <string>

class SyncServer {
public:
    SyncServer(boost::asio::io_context& io, short port);
    void run();

private:
    boost::asio::ip::tcp::acceptor acceptor_;
    std::string process(const std::string& input);
};#pragma once
