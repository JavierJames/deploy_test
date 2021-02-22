#include <stdio.h>
#include "PICCommander.hpp"
#include "gtest/gtest.h"
#include "hal/hal-tdd/HAL_TDD.hpp"
#include "hal/hal-tdd/RTOSTask_TDD.hpp"

using ::testing::_;

namespace {

// Tests factorial of negative numbers.
TEST(PICTest, Fake) {
    HAL& hal = getHAL();

    PICCommander p;
    EXPECT_CALL(p, create(_, _, _));
    p.start();
}
}  // namespace
