#include <locale.h>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "computer_room.h"

/**
 * @brief Главная функция программы
 * 
 * Создает компьютерный класс, запускает потоки студентов, отслеживает завершение и выводит статистику.
 * 
 * @return 0 при успешном завершении программы
 */
int main() {
    setlocale(LC_ALL, "Russian");

    std::cout << "Запуск симуляции компьютерного класса\n\n";
    std::cout << "• Группа КС-40: 30 студентов (требуется 15 для начала)\n";
    std::cout << "• Группа КС-44: 24 студента (требуется 12 для начала)\n";
    std::cout << "• Вместимость класса: 20 студентов\n";
    std::cout << "• Цель: каждому студенту посетить 2 занятия\n\n\n";

    ComputerRoom room;
    std::vector<std::thread> threads;

    // Создание потоков для группы КС-40
    std::cout << "Запуск потоков для группы КС-40  \n";
    for (int i = 0; i < 30; ++i) {
        threads.emplace_back([&room, i]() {
            room.student_behavior(1, i);
        });
    }

    // Создание потоков для группы КС-44
    std::cout << "Запуск потоков для группы КС-44  \n";
    for (int i = 0; i < 24; ++i) {
        threads.emplace_back([&room, i]() {
            room.student_behavior(2, i);
        });
    }

    std::cout << "\nОжидание завершения всех посещений...\n";
    std::cout << "   (максимальное время ожидания: 200 секунд)\n\n";

    // Ожидание завершения всех посещений с таймаутом
    auto start = std::chrono::steady_clock::now();
    while (!room.all_students_completed()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start).count();
        
        if (elapsed >= 200) {
            std::cout << "\nДостигнут таймаут ожидания (200 секунд). Принудительная остановка.\n";
            break;
        }
        
        // Вывод прогресса каждые 5 секунд
        if (elapsed % 5 == 0 && elapsed > 0) {
            std::cout << "   Прошло " << elapsed << " секунд...\n";
        }
    }

    // Остановка всех потоков
    std::cout << "\nЗавершение работы всех потоков...\n";
    room.stop();

    // Ожидание завершения всех потоков
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    // Вывод статистики
    room.print_statistics();

    std::cout << "\nПрограмма успешно завершена\n";
    return 0;
}
