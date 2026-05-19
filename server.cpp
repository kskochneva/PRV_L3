#include "server.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <thread>
//реализация сервера 

Session::Session(tcp::socket socket, asio::io_context& io_context)
    : socket_(std::move(socket)), io_context_(io_context), timer_(io_context) {}

void Session::start() {
    do_read();  // начинаем читать от клиента
}

void Session::do_read() {
    auto self = shared_from_this();  // держим объект живым
    asio::async_read_until(socket_, read_buffer_, '\n',
        [this, self](error_code ec, size_t /*length*/) {
            if (!ec) {
                // Извлекаем строку из буфера
                std::istream is(&read_buffer_);
                std::string request;
                std::getline(is, request);
                
                // Удаляем символ возврата каретки, если есть
                if (!request.empty() && request.back() == '\r') {
                    request.pop_back();
                }
                
                std::cout << "[SERVER] Received: \"" << request << "\" from client "
                          << socket_.remote_endpoint().address().to_string() << std::endl;
                
                process_request(request);
            } else {
                std::cerr << "[SERVER] Read error: " << ec.message() << std::endl;
            }
        });
}

void Session::process_request(const std::string& request) {
    //  Задача 1: Обработка обычного сообщения
    // вывод строка КАПСЛОКОМ
    
    // Проверяем, не является ли запрос командой "timer N"
    if (request.rfind("timer ", 0) == 0) {  // начинается с "timer "
        try {
            int seconds = std::stoi(request.substr(6));  // "timer 5" -> 5
            if (seconds > 0) {
                std::string confirm = "Ready in " + std::to_string(seconds) + " sec\n";
                do_write(confirm);
                handle_timer("Done!\n", seconds);
                return;
            }
        } catch (...) {} //try catch для игнор аошибок 
        //ловим любое исключен 
    }
    
    //  Задача 2: Асинхронное вычисление максимума 
    // в строке числа через пробел 3 4 5 
    //проверяем числа через пробел 
    bool is_number_list = true;
    for (char c : request) {
        if (!std::isdigit(c) && c != ' ' && c != '-') {
            is_number_list = false;
            break;
        }
    }
    
    if (is_number_list && !request.empty()) {
        // Асинхронно вычисляем максимум (через post)
        auto self = shared_from_this();//увеличиваем счетчик сслыок 
        //оябъект есть пока callback не закончился 
        //для ситуации когда объект уничтожен во время сессии
        std::string request_copy = request;
        
        asio::post(io_context_, [this, self, request_copy]() {
            // Эта задача выполнится в потоке io_context, но не блокирует чтение
            std::istringstream iss(request_copy);//поток чтения сротки 
            int max_val = std::numeric_limits<int>::min();
            //мини возможное значение для типа инт
            //для коррекной работы на отрицательных числа
            
            int num;
            bool has_number = false;
            
            while (iss >> num) {
                // оператора << возвращает поток 
                has_number = true;
                if (num > max_val) max_val = num;
            }
            
            std::string response;
            if (has_number) {
                response = "Max: " + std::to_string(max_val) + "\n";
            } else {
                response = "Error: no numbers found\n";
            }
            
            // Возвращаем результат клиенту
            do_write(response);
        });
        return;
    }
    
    //  Задача 1 (обычный эхо-режим) 
    //эхо режим клиент привет сервер привет 
    std::string upper_str = request;
    std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    
    std::string response = std::to_string(request.length()) + ": " + upper_str + "\n";
    do_write(response);
}

void Session::do_write(const std::string& response) {
    auto self = shared_from_this();
    asio::async_write(socket_, asio::buffer(response),
        [this, self](error_code ec, size_t /*bytes*/) {
            if (!ec) {
                std::cout << "[SERVER] Sent response" << std::endl;
                // После отправки ответа снова читаем следующее сообщение
                do_read();
            } else {
                std::cerr << "[SERVER] Write error: " << ec.message() << std::endl;
            }
        });
}

void Session::handle_timer(const std::string& response, int seconds) {
    auto self = shared_from_this();
    timer_.expires_after(std::chrono::seconds(seconds));
    timer_.async_wait([this, self, response](error_code ec) {
        if (!ec) {
            // Таймер сработал — отправляем сообщение
            asio::async_write(socket_, asio::buffer(response),
                [this, self](error_code ec, size_t) {
                    if (!ec) {
                        std::cout << "[SERVER] Timer message sent" << std::endl;
                    }
                });
        }
    });
}

// при менение сервера 

Server::Server(asio::io_context& io_context, unsigned short port)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    std::cout << "[SERVER] Listening on port " << port << std::endl;
}

void Server::start() {
    do_accept();
}

void Server::do_accept() {
    acceptor_.async_accept(
        [this](error_code ec, tcp::socket socket) {
            if (!ec) {
                std::cout << "[SERVER] New connection from "
                          << socket.remote_endpoint().address().to_string() << std::endl;
                
                // Создаём сессию для обслуживания клиента
                auto session = std::make_shared<Session>(std::move(socket), io_context_);
                session->start();
            } else {
                std::cerr << "[SERVER] Accept error: " << ec.message() << std::endl;
            }
            
            // Продолжаем принимать новые подключения
            do_accept();
        });
}
