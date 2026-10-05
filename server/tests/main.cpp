#include <tests-util.hpp>

#include "logger.hpp"
#include <spdlog/sinks/basic_file_sink.h>

namespace {

class TestLogListener : public EmptyTestEventListener {
public:
  void OnTestStart(const TestInfo &info) override {
    spdlog::info("\n"
                 "==============================\n"
                 "START TEST: {}\n"
                 "==============================",
                 getTestID(info));
  }

  void OnTestEnd(const TestInfo &info) override {
    spdlog::info("\n"
                 "==============================\n"
                 "END TEST: {}\n"
                 "==============================",
                 getTestID(info));
  }
};

} // namespace

int main(int argc, char **argv) {
  // Clear test output directory.
  std::filesystem::remove_all(TestOutputDir);
  std::filesystem::create_directory(TestOutputDir);

  InitializeLogging(false, TestOutputDir, 0);

  InitGoogleTest(&argc, argv);

  UnitTest::GetInstance()->listeners().Append(
      new TestLogListener); // GoogleTest takes ownership.

  return RUN_ALL_TESTS();
}