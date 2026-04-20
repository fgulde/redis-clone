//
// Created by fguld on 4/20/2026.
//

#include <gtest/gtest.h>

extern bool g_logging_enabled;

int main(int argc, char** argv) {
  g_logging_enabled = false;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}