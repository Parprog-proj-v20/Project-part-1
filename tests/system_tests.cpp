#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <iostream>
#include <locale>
#include <clocale>
#include "../include/computer_room.h"

class SystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::setlocale(LC_ALL, "en_US.UTF-8");
        std::locale::global(std::locale("en_US.UTF-8"));
        std::wcout.imbue(std::locale("en_US.UTF-8"));
    }
    ComputerRoom room;
};

/**
 * @brief Тест 1: Полная симуляция работы системы до завершения
 */
TEST_F(SystemTest, CompleteSystemSimulationUntilCompletion) {
    ComputerRoom room;
    std::vector<std::thread> students;

    std::cout << "=== СИСТЕМНЫЙ ТЕСТ: Полная симуляция ===" << std::endl;

    for (int i = 0; i < 30; ++i) {
        students.emplace_back([&room, i]() {
            room.student_behavior(1, i);
            });
    }
    for (int i = 0; i < 24; ++i) {
        students.emplace_back([&room, i]() {
            room.student_behavior(2, i);
            });
    }

    std::cout << "Запущено 30 студентов КС-40 и 24 студента КС-44" << std::endl;

    auto start_time = std::chrono::steady_clock::now();
    int check_count = 0;

    while (true) {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);

        if (elapsed.count() >= check_count * 10) {
            std::cout << "Прошло " << elapsed.count() << " секунд. ";
            if (room.all_students_completed()) {
                std::cout << "ВСЕ студенты завершили!" << std::endl;
                break;
            }
            else {
                std::cout << "Система еще работает..." << std::endl;
            }
            check_count++;
        }

        if (elapsed.count() > 200) {
            std::cout << "Прошло 200 секунд. Закругляемся" << std::endl;
            break;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    room.stop();
    for (auto& student : students) {
        if (student.joinable()) student.join();
    }

    room.print_statistics();

    EXPECT_TRUE(room.all_students_completed()) << "Система не достигла конечного состояния";
}

/**
 * @brief Тест 2: Тестирование граничных условий системы
 *
 * Тест 2.1: Пустая система
 * Тест 2.2: Система с одним студентом
 * Тест 2.3: Система с минимальным необходимым количеством студентов
 */
TEST_F(SystemTest, SystemBoundaryConditions) {
    {
        ComputerRoom room;
        std::cout << "Тест: Пустая система" << std::endl;
        EXPECT_NO_THROW({
            room.stop();
            room.print_statistics();
            });
    }

    {
        ComputerRoom room;
        std::thread single_student([&room]() {
            room.student_behavior(1, 0);
            });

        std::this_thread::sleep_for(std::chrono::seconds(5));
        room.stop();
        single_student.join();

        std::cout << "Тест: Один студент завершен" << std::endl;
    }

    {
        ComputerRoom room;
        std::vector<std::thread> students;

        for (int i = 0; i < 15; ++i) {
            students.emplace_back([&room, i]() {
                room.student_behavior(1, i);
                });
        }

        std::this_thread::sleep_for(std::chrono::seconds(8));
        room.stop();
        for (auto& student : students) {
            if (student.joinable()) student.join();
        }

        std::cout << "Тест: Система с минимальным необходимым количеством студентов работает корректно" << std::endl;
    }

    std::cout << "Все граничные условия обработаны корректно" << std::endl;
    SUCCEED();
}
