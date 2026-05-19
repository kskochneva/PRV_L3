#pragma once

#include <boost/asio.hpp>
#include <string>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using error_code = boost::system::error_code;

class Client {
public:
    Client(asio::io_context& io_context, const std::string& host, unsigned short port);
    
    // Синхронная отправка сообщения и получение ответа
    std::string send_request(const std::string& request);
    
    // Подключение к серверу
    bool connect();

private:
    asio::io_context& io_context_;
    tcp::socket socket_;
    std::string host_;
    unsigned short port_;
};
