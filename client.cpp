#include "client.hpp"
#include <iostream>
#include <boost/asio/connect.hpp>

//конструктор клиента
//ссылка на гланый цикл собыйти асио
//адрес и пор т сервера 
Client::Client(asio::io_context& io_context, const std::string& host, unsigned short port)
    : io_context_(io_context), socket_(io_context), host_(host), port_(port) {}
//сохраняем ссылку создаем сокет сохраняем алрес и порт 

bool Client::connect() {
    try {
        //резолвер объект который прелбразует доменное имя и порт в спсиок айпи 
        tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(host_, std::to_string(port_));
        //возвращает список возмодных конечных точек
        asio::connect(socket_, endpoints);//пытается подклчиться к каждой тконечной точке из списка пока не получится
        std::cout << "[CLIENT] Connected to " << host_ << ":" << port_ << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[CLIENT] Connection failed: " << e.what() << std::endl;
        return false;
    }
}

std::string Client::send_request(const std::string& request) {
    try {
        // Отправляем запрос + символ новой строки
        std::string msg = request + "\n";
        asio::write(socket_, asio::buffer(msg));
        std::cout << "[CLIENT] Sent: \"" << request << "\"" << std::endl;
        
        // Читаем ответ до символа новой строки
        asio::streambuf response_buf;
        asio::read_until(socket_, response_buf, '\n');
        //извлекаем отвте 
        std::istream is(&response_buf);//создаем поток для чтени буфера 
        std::string response;
        std::getline(is, response);
        //Сервер отправил: "11: HELLO WORLD\n"
                        
//response_buf содержит: "11: HELLO WORLD\n"
        
        // Удаляем символ возврата каретки, если есть
        if (!response.empty() && response.back() == '\r') {
            response.pop_back();
        }
        
        return response;
    } catch (const std::exception& e) {
        std::cerr << "[CLIENT] Error: " << e.what() << std::endl;
        return "ERROR: " + std::string(e.what());
    }
}
