#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <mutex>
#include <set>
#include <iostream>
#include <locale>
#include <clocale>
#include "../include/computerRoom.h"

class ThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::setlocale(LC_ALL, "en_US.UTF-8");
        std::locale::global(std::locale("en_US.UTF-8"));
        std::wcout.imbue(std::locale("en_US.UTF-8"));
    }
    void TearDown() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};

/**
 * @brief Тест 1: Проверяем, что нет гонки данных при параллельном доступе
 */
TEST_F(ThreadSafetyTest, NoRaceConditionOnStateAccess) {
    ComputerRoom room;
    std::vector<std::thread> threads;
    std::atomic<int> successful_operations{ 0 };
    const int TOTAL_OPERATIONS = 1000;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&, total_ops = TOTAL_OPERATIONS]() {
            for (int j = 0; j < total_ops / 10; ++j) {
                room.allStudentsCompleted();
                successful_operations++;
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(successful_operations, TOTAL_OPERATIONS);
    EXPECT_NO_THROW(room.printStatistics());
}

/**
 * @brief Тест 2: Проверка безопасности методов статистики
 * 
 * Проверяем, что printStatistics и allStudentsCompleted безопасны
 */
TEST_F(ThreadSafetyTest, StatisticsMethodsThreadSafe) {
    ComputerRoom room;
    std::vector<std::thread> threads;
    std::atomic<bool> stop_flag{ false };
    std::atomic<int> exceptions_count{ 0 };

    const int READER_THREADS = 10;

    for (int i = 0; i < READER_THREADS; ++i) {
        threads.emplace_back([&room, &stop_flag, &exceptions_count, i]() {
            while (!stop_flag) {
                try {
                    if (i % 2 == 0) {
                        room.allStudentsCompleted();
                    }
                    else {
                        room.printStatistics();
                    }
                }
                catch (const std::exception& e) {
                    exceptions_count++;
                    std::cerr << "Exception in thread " << i << ": " << e.what() << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            });
    }

    std::vector<std::thread> students;
    for (int i = 0; i < 5; ++i) {
        students.emplace_back([&room, i]() {
            room.studentBehavior(1, i);
            });
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    stop_flag = true;
    room.stop();

    for (auto& thread : threads) {
        thread.join();
    }
    for (auto& student : students) {
        student.join();
    }

    EXPECT_EQ(exceptions_count, 0);
}

/**
 * @brief Тест 3: Отсутствие deadlock при сложных сценариях
 */
TEST_F(ThreadSafetyTest, NoDeadlockInComplexScenarios) {
    ComputerRoom room;
    std::vector<std::thread> threads;
    std::atomic<bool> test_completed{ false };
    std::atomic<int> deadlock_detected{ 0 };

    const int THREAD_COUNT = 15;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&room, i, &test_completed, &deadlock_detected]() {
            auto start_time = std::chrono::steady_clock::now();

            while (!test_completed) {
                switch ((i + rand()) % 4) {
                case 0:
                    room.studentBehavior(1, i % 30);
                    break;
                case 1:
                    room.studentBehavior(2, i % 24);
                    break;
                case 2:
                    room.allStudentsCompleted();
                    break;
                case 3:
                    room.printStatistics();
                    break;
                }

                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time);
                if (elapsed.count() > 10) {
                    deadlock_detected++;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            });
    }

    std::this_thread::sleep_for(std::chrono::seconds(8));
    test_completed = true;
    room.stop();

    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    EXPECT_EQ(deadlock_detected, 0);
}

/**
 * @brief Тест 4: Проверка безопасности потоков при вызове stop из многих потоков
 */
TEST_F(ThreadSafetyTest, ConcurrentStopCalls) {
    ComputerRoom room;
    std::vector<std::thread> students;
    std::vector<std::thread> stoppers;
    std::atomic<int> stop_calls_count{ 0 };
    const int STOPPER_THREADS = 10;

    for (int i = 0; i < 10; ++i) {
        students.emplace_back([&room, i]() {
            room.studentBehavior(1, i);
            });
    }

    for (int i = 0; i < STOPPER_THREADS; ++i) {
        stoppers.emplace_back([&room, &stop_calls_count]() {
            for (int j = 0; j < 5; ++j) {
                room.stop();
                stop_calls_count++;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            });
    }

    for (auto& stopper : stoppers) {
        stopper.join();
    }

    room.stop();
    for (auto& student : students) {
        if (student.joinable()) {
            student.join();
        }
    }

    EXPECT_GE(stop_calls_count, STOPPER_THREADS * 5);
    EXPECT_NO_THROW(room.printStatistics());
}
