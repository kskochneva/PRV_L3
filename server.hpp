
//объявление сервера 
#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <vector>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using error_code = boost::system::error_code;

//  обслуживаем одного клиента (одно соединение)
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, asio::io_context& io_context);
    void start();  // начало обслуживания клиента

private:
    void do_read();                    // чтение запроса от клиента
    void process_request(const std::string& request);  // обработка запроса
    void do_write(const std::string& response);        // отправка ответа
    void handle_timer(const std::string& response, int seconds);  // отложенный ответ

    tcp::socket socket_;
    asio::io_context& io_context_;
    asio::streambuf read_buffer_;      // буфер для чтения
    asio::steady_timer timer_;         // таймер для задачи 3
};

// Класс сервера (принимает подключения)
class Server {
public:
    Server(asio::io_context& io_context, unsigned short port);
    void start();

private:
    void do_accept();  // асинхронное принятие подключения

    asio::io_context& io_context_;
    tcp::acceptor acceptor_;
};
