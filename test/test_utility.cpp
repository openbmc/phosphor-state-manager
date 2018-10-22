#include <gtest/gtest.h>
#include "../utility.hpp"

using namespace utility;

TEST(UtilityTest, TestConvertGPIOToNum)
{
    /* len should be 2 or more */
    EXPECT_EQ(convertGpioToNum(""), -1);

    /* Last char should be a digit */
    EXPECT_EQ(convertGpioToNum("AB"), -1);

    /* First char should be alpha char*/
    EXPECT_EQ(convertGpioToNum("1C"), -1);

    /* Check correct 2 length string */
    EXPECT_EQ(convertGpioToNum("D2"), 26);

    /* Check correct 3 length string */
    EXPECT_EQ(convertGpioToNum("EF3"), 1283);
}

TEST(UtilityTest, TestGPIOSetValue)
{
    //TODO
}