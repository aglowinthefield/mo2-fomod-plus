#include <gtest/gtest.h>
#include "../src/util/stringutil.h"

TEST(StringUtil, Trim_Copy) {
  const std::string str = "  extra spaces  ";
  EXPECT_EQ(trim_copy(str), "extra spaces");
}
