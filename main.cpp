#include "server.hpp"
#include "client.hpp"
#include <iostream>
#include <thread>
#include <chrono>

void run_server(asio::io_context& io_context, unsigned short port) {
    Server server(io_context, port);
    server.start();
    io_context.run();  // блокируе текущий поток
    //запускается бескоенчный цикл обработки событий 
}

int main() {
    const unsigned short PORT = 12345;
    const std::string HOST = "127.0.0.1";
    
    // Запускаем сервер в отдельном потоке
    asio::io_context server_io;
    std::thread server_thread([&server_io, PORT]() {
        run_server(server_io, PORT);
    });
    
    // Даём серверу время на запуск
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Создаём клиента
    asio::io_context client_io;
    Client client(client_io, HOST, PORT);
    
    if (!client.connect()) {
        std::cerr << "Failed to connect to server" << std::endl;
        server_io.stop();
        server_thread.join();
        return 1;
    }
    
  
    
    // Задача 1
    std::cout << "\n--- Task 1: Echo with length and uppercase ---" << std::endl;
    std::string response1 = client.send_request("Hello world");
    std::cout << "[CLIENT] Response: \"" << response1 << "\"" << std::endl;
    
    // Задача 2: Асинхронное  максимум
    std::cout << "\n--- Task 2: Async max calculation ---" << std::endl;
    std::string response2 = client.send_request("1 88 5 13 42 7");
    std::cout << "[CLIENT] Response: \"" << response2 << "\"" << std::endl;
    
    // Задача 3: Таймер
    std::cout << "\n--- Task 3: Timer test (5 seconds) ---" << std::endl;
    std::string response3 = client.send_request("timer 5");
    std::cout << "[CLIENT] Immediate response: \"" << response3 << "\"" << std::endl;
    
    // Ждём сообщение от таймера (оно придёт асинхронно)
    std::cout << "[CLIENT] Waiting for timer message (5 seconds)..." << std::endl;
    
    // Чтобы получить асинхронное сообщение от таймера, нужно продолжать читать
   
    try {
        asio::streambuf timer_buf;
        asio::read_until(client.socket_, timer_buf, '\n');
        std::istream is(&timer_buf);
        std::string timer_msg;
        std::getline(is, timer_msg);
        std::cout << "[CLIENT] Timer response: \"" << timer_msg << "\"" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "[CLIENT] Timer read: " << e.what() << std::endl;
    }
    
    //  несколько чисел
    std::cout << "\n--- Additional test: max of negative numbers ---" << std::endl;
    std::string response4 = client.send_request("-10 -5 -30 -2");
    std::cout << "[CLIENT] Response: \"" << response4 << "\"" << std::endl;
    
    std::cout << "\n--- All tests completed ---" << std::endl;
    
    // Завершаем работу
    server_io.stop();
    server_thread.join();
    
    return 0;
}
