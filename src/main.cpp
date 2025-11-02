#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "computer_room.h"
#include <windows.h>

/**
 * @brief Главная функция программы
 * 
 * Создает компьютерный класс, запускает потоки студентов, отслеживает завершение и выводит статистику.
 * 
 * @return 0 при успешном завершении программы
 */
int main() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    std::cout << std::string(60, '*') << "\n\n";
    std::cout << "> Группа КС-40: 30 студентов (требуется 15 для начала)\n";
    std::cout << "> Группа КС-44: 24 студента (требуется 12 для начала)\n";
    std::cout << "> Вместимость класса: 20 студентов\n\n";
    std::cout << std::string(60, '*') << "\n\n";

    ComputerRoom room;
    std::vector<std::thread> threads;

    // Создание потоков для группы КС-40
    std::cout << "\t! Запуск потоков для группы КС-40\n";
    for (int i = 0; i < 30; ++i) {
        threads.emplace_back([&room, i]() {
            room.student_behavior(1, i);
        });
    }

    // Создание потоков для группы КС-44
    std::cout << "\t! Запуск потоков для группы КС-44\n";
    for (int i = 0; i < 24; ++i) {
        threads.emplace_back([&room, i]() {
            room.student_behavior(2, i);
        });
    }

    // Ожидание завершения всех посещений с таймаутом
    auto start = std::chrono::steady_clock::now();
    while (!room.all_students_completed()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start).count();
        
        if (elapsed >= 200) {
            std::cout << "\n! Достигнут таймаут ожидания (200 секунд).\n";
            break;
        }
        
        // Вывод прогресса каждые 5 секунд
        if (elapsed % 5 == 0 && elapsed > 0) {
            std::cout << std::string(60, '-') << "\n";
            std::cout << "\t! Время работы программы: " << elapsed << " секунд\n";
            std::cout << std::string(60, '-') << "\n";
        }
    }

    // Остановка всех потоков
    std::cout << "\n\t! Завершение работы всех потоков\n\n";
    room.stop();

    // Ожидание завершения всех потоков
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    // Вывод статистики
    room.print_statistics();
    return 0;
}
