#include "server.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <limits>
#include <chrono>

// ==================== Session Implementation ====================

Session::Session(tcp::socket socket, asio::io_context& io_context)
    : socket_(std::move(socket)),
      io_context_(io_context),
      timer_(io_context),
      strand_(io_context.get_executor()) {
}

void Session::start() {
    do_read();
}

// Задача 4: потокобезопасное логирование через strand
void Session::log_message(const std::string& msg) {
    asio::post(strand_, [this, msg]() {
        std::cout << "[SESSION] " << msg << std::endl;
    });
}

void Session::do_read() {
    auto self = shared_from_this();
    
    asio::async_read_until(socket_, read_buffer_, '\n',
        [this, self](error_code ec, size_t /*length*/) {
            if (!ec) {
                // Извлекаем строку из буфера
                std::istream is(&read_buffer_);
                std::string request;
                std::getline(is, request);
                
                // Удаляем символ возврата каретки (если есть)
                if (!request.empty() && request.back() == '\r') {
                    request.pop_back();
                }
                
                log_message("Received: \"" + request + "\"");
                process_request(request);
            } else {
                log_message("Read error: " + ec.message());
            }
        });
}

// ========== ОСНОВНАЯ ЛОГИКА ОБРАБОТКИ ЗАПРОСОВ (ВСЕ ЗАДАЧИ) ==========
void Session::process_request(const std::string& request) {
    // ========== ЗАДАЧА 3: АСИНХРОННЫЙ ТАЙМЕР ==========
    // Проверяем, начинается ли запрос с "timer "
    if (request.rfind("timer ", 0) == 0) {
        try {
            int seconds = std::stoi(request.substr(6));
            if (seconds > 0) {
                // Немедленный ответ
                std::string confirm = "Ready in " + std::to_string(seconds) + " sec\n";
                do_write(confirm);
                log_message("Timer started: " + std::to_string(seconds) + " seconds");
                
                // Асинхронный таймер
                handle_timer("Done!\n", seconds);
                return;
            }
        } catch (const std::exception& e) {
            log_message("Invalid timer command: " + request);
        }
    }
    
    // ========== ЗАДАЧА 2: АСИНХРОННОЕ ВЫЧИСЛЕНИЕ МАКСИМУМА ==========
    // Проверяем, состоит ли строка из цифр, пробелов и знаков минус
    bool is_number_list = true;
    for (char c : request) {
        if (!std::isdigit(static_cast<unsigned char>(c)) && c != ' ' && c != '-') {
            is_number_list = false;
            break;
        }
    }
    
    if (is_number_list && !request.empty()) {
        log_message("Number list detected, calculating max asynchronously");
        
        auto self = shared_from_this();
        std::string request_copy = request;
        
        // Задача 2: асинхронное вычисление через post()
        asio::post(io_context_, [this, self, request_copy]() {
            std::istringstream iss(request_copy);
            int max_val = std::numeric_limits<int>::min();
            int num;
            bool has_number = false;
            
            while (iss >> num) {
                has_number = true;
                if (num > max_val) {
                    max_val = num;
                }
            }
            
            std::string response;
            if (has_number) {
                response = "Max: " + std::to_string(max_val) + "\n";
                log_message("Max calculated: " + std::to_string(max_val));
            } else {
                response = "Error: no numbers found\n";
                log_message("No numbers found");
            }
            
            do_write(response);
        });
        
        return;
    }
    
    // ========== ЗАДАЧА 1: ЭХО С ДЛИНОЙ И ВЕРХНИМ РЕГИСТРОМ ==========
    log_message("Echo mode");
    
    // Преобразуем в верхний регистр
    std::string upper_str = request;
    std::transform(upper_str.begin(), upper_str.end(), upper_str.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    
    // Формируем ответ: длина + ": " + строка в верхнем регистре
    std::string response = std::to_string(request.length()) + ": " + upper_str + "\n";
    
    do_write(response);
}

void Session::do_write(const std::string& response) {
    auto self = shared_from_this();
    
    asio::async_write(socket_, asio::buffer(response),
        [this, self](error_code ec, size_t /*bytes*/) {
            if (!ec) {
                log_message("Response sent");
                do_read();  // снова читаем следующее сообщение
            } else {
                log_message("Write error: " + ec.message());
            }
        });
}

// Задача 3: обработка таймера
void Session::handle_timer(const std::string& response, int seconds) {
    auto self = shared_from_this();
    
    timer_.expires_after(std::chrono::seconds(seconds));
    timer_.async_wait([this, self, response](error_code ec) {
        if (!ec) {
            log_message("Timer expired, sending message");
            asio::async_write(socket_, asio::buffer(response),
                [this, self](error_code ec, size_t) {
                    if (!ec) {
                        log_message("Timer message sent");
                    }
                });
        } else if (ec != asio::error::operation_aborted) {
            log_message("Timer error: " + ec.message());
        }
    });
}

// ==================== Server Implementation ====================

Server::Server(asio::io_context& io_context, unsigned short port)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      log_strand_(io_context.get_executor()) {
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
                
                auto session = std::make_shared<Session>(std::move(socket), io_context_);
                session->start();
            } else {
                std::cerr << "[SERVER] Accept error: " << ec.message() << std::endl;
            }
            
            do_accept();  // продолжаем принимать подключения
        });
}

// Задача 4: потокобезопасное добавление в лог через strand
void Server::add_to_log(const std::string& entry) {
    asio::post(log_strand_, [this, entry]() {
        log_.push_back(entry);
        std::cout << "[LOG] " << entry << std::endl;
    });
}

// Задача 4: получение лога
std::vector<std::string> Server::get_log() const {
    std::lock_guard<std::mutex> lock(log_mutex_);
    return log_;
}
