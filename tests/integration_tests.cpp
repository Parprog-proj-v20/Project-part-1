#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "../src/computer_room.h"

class IntegrationTest : public ::testing::Test {
protected:
    ComputerRoom room;
};


/**
 * @brief Тест 1: Проверяет, что статистика работает после работы студентов
 */
TEST_F(IntegrationTest, StatisticsAfterStudentActivity) {
    std::thread student([this]) {
        room.student_bahavior(1, 0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    room.stop();
    student.join();

    EXPECT_NO_THROW({
        room.print_statistics();
        });
}