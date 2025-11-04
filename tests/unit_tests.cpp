#include <gtest/gtest.h>
#include <locale>
#include <clocale>
#include "../include/computerRoom.h"

class UnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::setlocale(LC_ALL, "en_US.UTF-8");
        std::locale::global(std::locale("en_US.UTF-8"));
        std::wcout.imbue(std::locale("en_US.UTF-8"));
    }
    ComputerRoom room;
};

/**
 * @brief Тест 1: Проверка начального состояния
 *
 * Тестируем метод allStudentsCompleted изолированно
 */
TEST_F(UnitTest, InitialStateNoOneCompleted) {
    EXPECT_FALSE(room.allStudentsCompleted());
}

/**
 * @brief Тест 2: Многократный вызов статистики
 */
TEST_F(UnitTest, MultipleStatisticsCalls) {
    EXPECT_NO_THROW({
        room.printStatistics();
        room.printStatistics();
        room.printStatistics();
        });
}

/**
 * @brief Тест 3: Stop метод работает без ошибок
 */
TEST_F(UnitTest, StopMethodWorks) {
    EXPECT_NO_THROW({
        room.stop();
        });
}

/**
 * @brief Тест 4: Повторный вызов stop безопасен
 */
TEST_F(UnitTest, MultipleStopCallsAreSafe) {
    room.stop();
    room.stop();
    SUCCEED();
}
