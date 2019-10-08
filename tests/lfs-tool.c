#include "unity_fixture.h"

TEST_GROUP(LfsTool);

TEST_SETUP(LfsTool)
{}

TEST_TEAR_DOWN(LfsTool)
{}

TEST(LfsTool, LfsToolMain)
{
    TEST_FAIL_MESSAGE("define me");
}

TEST_GROUP_RUNNER(LfsTool)
{
    RUN_TEST_CASE(LfsTool, LfsToolMain);
}
