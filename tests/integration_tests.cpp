#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <locale>
#include <clocale>
#include "../include/computer_room.h"

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::setlocale(LC_ALL, "en_US.UTF-8");
        std::locale::global(std::locale("en_US.UTF-8"));
        std::wcout.imbue(std::locale("en_US.UTF-8"));
    }
    ComputerRoom room;
};


/**
 * @brief Тест 1: Проверяет, что статистика работает после работы студентов
 */
TEST_F(IntegrationTest, StatisticsAfterStudentActivity) {
    std::thread student([this]() {
        room.student_behavior(1, 0);
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    room.stop();
    student.join();

    EXPECT_NO_THROW({
        room.print_statistics();
        });
}

/**
 * @brief Тест 2: Проверяет, функциональность начала занятий при достаточном количестве студентов
 *
 * Запускаем достаточно студентов для начала занятий
 */
TEST_F(IntegrationTest, ClassStartsWhenEnoughStudents) {
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
 * @brief Тест 3: Группы работают в разное время
 *
 * Сначала запускаем только КС-40. Затем запускаем КС-44 (после того как КС-40 завершит)
 */
TEST_F(IntegrationTest, GroupsWorkAtDifferentTimes) {
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
 * @brief Тест 4: Проверяет корректность завершения во время занятия
 */
TEST_F(IntegrationTest, GracefulShutdownDuringActiveClass) {
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

/**
 * @brief Тест 5: Проверяет корректность работы вытеснения  студентов при начале занятия
 *
 * Сначала запускаем некоторое количество студентов из одной группы, затем запускаем количество студентов другой группы, достаточное, чтобы начать занятие.
 */
TEST_F(IntegrationTest, StudentsGetEvictedWhenOtherGroupStartsClass) {
    std::vector<std::thread> students;

    for (int i = 0; i < 5; ++i) {
        students.emplace_back([this, i]() {
            room.student_behavior(2, i);
            });
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    for (int i = 0; i < 15; ++i) {
        students.emplace_back([this, i]() {
            room.student_behavior(1, i);
            });
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    room.stop();
    for (auto& student : students) {
        if (student.joinable()) student.join();
    }

    SUCCEED();
}
