//
// Created by fguld on 4/20/2026.
//

#include <gtest/gtest.h>
#include "../src/util/Logger.hpp"

int main(int argc, char** argv) {
  Logger::setEnabled(false);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}