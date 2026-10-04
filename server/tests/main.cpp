#include <tests-util.hpp>

int main(int argc, char** argv) {
  // Clear test output directory.
  std::filesystem::remove_all(TestOutputDir);
  std::filesystem::create_directory(TestOutputDir);

  InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}