#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include "../src/computer_room.h"

class FunctionalTest : public ::testing::Test {
protected:
    ComputerRoom room;
};

/**
 * @brief Тест 1: Проверяет, функциональность начала занятий при достаточном количестве студентов
 *
 * Запускаем достаточно студентов для начала занятий
 */
TEST_F(FunctionalTest, ClassStartsWhenEnoughStudents) {
    std::vector<std::thread> students;

    for (int i = 0; i < 15; ++i) {
        students.emplace_back([this, i]() {
            room.student_behavior(1, i);
            });
    }

    std::this_thread::sleep_for(std::chrono::seconds(8));

    room.stop();
    for (auto& student : students) {
        if (student.joinable()) student.join();
    }

    SUCCEED();
}

/**
 * @brief Тест 2: Группы работают в разное время
 *
 * Сначала запускаем только КС-40. Затем запускаем КС-44 (после того как КС-40 завершит)
 */
TEST_F(FunctionalTest, GroupsWorkAtDifferentTimes) {
    std::vector<std::thread> students;

    for (int i = 0; i < 15; ++i) {
        students.emplace_back([this, i]() {
            room.student_behavior(1, i); 
            });
    }

    std::this_thread::sleep_for(std::chrono::seconds(8));

    for (int i = 0; i < 12; ++i) {
        students.emplace_back([this, i]() {
            room.student_behavior(2, i); 
            });
    }

    std::this_thread::sleep_for(std::chrono::seconds(8));

    room.stop();
    for (auto& student : students) {
        if (student.joinable()) student.join();
    }

    SUCCEED();
}

/**
 * @brief Тест 3: Проверяет корректность завершения во время занятия
 */
TEST_F(FunctionalTest, GracefulShutdownDuringActiveClass) {
    std::vector<std::thread> students;

    for (int i = 0; i < 20; ++i) {
        students.emplace_back([this, i]() {
            room.student_behavior(1, i % 15); 
            });
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
    room.stop();

    for (auto& student : students) {
        EXPECT_TRUE(student.joinable());
        student.join();
    }

    SUCCEED();
}