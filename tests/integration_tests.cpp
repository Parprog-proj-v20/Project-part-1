#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "../src/computer_room.h"

class IntegrationTest : public ::testing::Test {
protected:
    ComputerRoom room;
};