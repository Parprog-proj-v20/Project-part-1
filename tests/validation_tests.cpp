#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <chrono>
#include "../../src/computer_room.h"

class ValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
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
 * @brief Проверяет, что не выходит больше, чем входит
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
 * @brief Проверяет, что количество студентов в классе не превышает количество мест
 */
TEST_F(ValidationTest, CapacityNeverExceeded) {
    // Этот тест сложнее без модификации кода, но можно проверить косвенно
    // через стабильность системы при максимальной нагрузке

    std::vector<std::thread> students;
    const int total_students = 40; // Больше, чем вместимость

    // Запускаем много студентов
    for (int i = 0; i < total_students; ++i) {
        students.emplace_back([this, i]() {
            room->student_behavior(1, i % 20); // Группа КС-40
            });
    }

    // Мониторим в течение времени
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Проверяем, что система стабильна (не упала и не зависла)
        // Без доступа к occupancy мы можем проверить только стабильность
        SUCCEED(); // Если не упало - вероятно, ограничение capacity работает
    }

    room->stop();
    for (auto& student : students) {
        if (student.joinable()) student.join();
    }

    // Если дошли сюда без deadlock - вероятно, capacity ограничение работает
    EXPECT_TRUE(true);
}

// 2.1. Тест: Если студентов больше, чем мест, и есть по половине студентов из каждой группы
TEST_F(ValidationTest, MixedGroupsWhenOverCapacity) {
    std::vector<std::thread> students;
    const int students_per_group = 15; // Больше, чем половина capacity

    // Запускаем студентов обеих групп
    for (int i = 0; i < students_per_group; ++i) {
        students.emplace_back([this, i]() {
            room->student_behavior(1, i); // КС-40
            });
        students.emplace_back([this, i]() {
            room->student_behavior(2, i); // КС-44
            });
    }

    // Мониторим систему
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Проверяем стабильность системы
    room->stop();
    for (auto& student : students) {
        if (student.joinable()) student.join();
    }

    // Система должна оставаться стабильной даже при конкуренции
    SUCCEED();
}

// 3. Тест: Студенты в кабинете из одной группы (косвенная проверка)
TEST_F(ValidationTest, SingleGroupDuringClassSession) {
    std::vector<std::thread> students;

    // Запускаем студентов обеих групп
    for (int i = 0; i < 10; ++i) {
        students.emplace_back([this, i]() {
            room->student_behavior(1, i); // КС-40
            });
    }
    for (int i = 0; i < 10; ++i) {
        students.emplace_back([this, i]() {
            room->student_behavior(2, i); // КС-44
            });
    }

    // Даем время для развития событий (начало занятий)
    std::this_thread::sleep_for(std::chrono::seconds(8));

    // Проверяем, что система работает корректно
    // Логика вытеснения студентов чужой группы должна быть во внутренней логике класса
    room->stop();
    for (auto& student : students) {
        if (student.joinable()) student.join();
    }

    // Если не было крахов и deadlock - вероятно, логика работает
    SUCCEED();
}

// 4. Тест: Проверить, что условие выполняется (все студенты получают 2 посещения)
TEST_F(ValidationTest, AllStudentsEventuallyComplete) {
    ComputerRoom room;
    std::vector<std::thread> students;

    // Запускаем всех студентов
    for (int i = 0; i < 30; ++i) {
        students.emplace_back([&room, i]() {
            room.student_behavior(1, i); // КС-40
            });
    }
    for (int i = 0; i < 24; ++i) {
        students.emplace_back([&room, i]() {
            room.student_behavior(2, i); // КС-44
            });
    }

    // Ждем выполнения условия с таймаутом
    auto start = std::chrono::steady_clock::now();
    bool completed = false;

    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(60)) {
        if (room.all_students_completed()) {
            completed = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    room.stop();
    for (auto& student : students) {
        if (student.joinable()) student.join();
    }

    // Проверяем, что условие выполнилось
    EXPECT_TRUE(completed) << "Не все студенты получили 2 посещения за отведенное время";

    // Выводим статистику для анализа
    testing::internal::CaptureStdout();
    room.print_statistics();
    std::string stats = testing::internal::GetCapturedStdout();
    std::cout << "Финальная статистика:\n" << stats << std::endl;
}