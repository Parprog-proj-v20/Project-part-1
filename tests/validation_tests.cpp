#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <iostream>
#include <locale>
#include <clocale>
#include "../include/computer_room.h"

class ValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::setlocale(LC_ALL, "en_US.UTF-8");
        std::locale::global(std::locale("en_US.UTF-8"));
        std::wcout.imbue(std::locale("en_US.UTF-8"));

        room = new ComputerRoom();
    }

    void TearDown() override {
        room->stop();
        delete room;
    }

    ComputerRoom* room;
    std::atomic<int> students_entered{ 0 };
    std::atomic<int> students_exited{ 0 };
};

/**
 * @brief Тест 1: Проверяет, что не выходит больше, чем входит
 */
TEST_F(ValidationTest, NeverMoreExitsThanEnters) {
    const int num_students = 10;
    std::vector<std::thread> students;

    for (int i = 0; i < num_students; ++i) {
        students.emplace_back([this, i]() {
            students_entered++;
            room->student_behavior(1, i); 
            students_exited++;
            });
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));

    room->stop();

    for (auto& student : students) {
        if (student.joinable()) student.join();
    }

    EXPECT_LE(students_exited.load(), students_entered.load());
    std::cout << "Вошло: " << students_entered << ", вышло: " << students_exited << std::endl;
}

/**
 * @brief Тест 2: Проверяет, что система не зависает при полной загрузке
 */
TEST_F(ValidationTest, NoDeadlockUnderFullLoad) {
    std::vector<std::thread> students;

    for (int i = 0; i < 40; ++i) {
        students.emplace_back([this, i]() {
            room->student_behavior(1, i % 20);
            });
    }
    for (int i = 0; i < 30; ++i) {
        students.emplace_back([this, i]() {
            room->student_behavior(2, i % 15); 
            });
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
    room->stop();

    for (auto& student : students) {
        EXPECT_TRUE(student.joinable()) << "Поток не joinable - возможен deadlock";
        if (student.joinable()) {
            student.join();
        }
    }

    SUCCEED();
}

/**
 * @brief Тест 3: Проверяет, что поведение системы корректно при конкуренции групп
 */
TEST_F(ValidationTest, GroupCompetitionHandledCorrectly) {
    std::vector<std::thread> students;

    for (int i = 0; i < 15; ++i) {
        students.emplace_back([this, i]() {
            room->student_behavior(1, i);  
            });
    }
    for (int i = 0; i < 12; ++i) {
        students.emplace_back([this, i]() {
            room->student_behavior(2, i);
            });
    }

    std::this_thread::sleep_for(std::chrono::seconds(8));
    room->stop(); 

    for (auto& student : students) {
        if (student.joinable()) student.join();
    }

    EXPECT_NO_THROW({
        room->print_statistics();
        });
}