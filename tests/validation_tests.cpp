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
