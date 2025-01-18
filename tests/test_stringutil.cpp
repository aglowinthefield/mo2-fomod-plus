#include <gtest/gtest.h>
#include "../share/stringutil.h"

TEST(StringUtil, Trim_Copy) {
  std::string str = "  extra spaces  ";
  EXPECT_EQ(trim(str), "extra spaces");
}
