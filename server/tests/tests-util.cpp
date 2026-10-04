#include <tests-util.hpp>

std::string getCurrentTestID() {
  const TestInfo *info = UnitTest::GetInstance()->current_test_info();
  const std::string suiteName = info->test_suite_name();
  const std::string testName = info->name();

  std::string id = suiteName + "." + testName;

  if (info->type_param() != nullptr) {
    id += "." + std::string(info->type_param());
  }
  if (info->value_param() != nullptr) {
    id += "." + std::string(info->value_param());
  }

  return id;
}

std::filesystem::path getTestOutputDirPath() {
  std::filesystem::path dirPath = TestOutputDir / getCurrentTestID();
  std::filesystem::create_directory(dirPath);
  return dirPath;
}

void saveImage(const cv::Mat &image, std::string name) {
  std::filesystem::path path = getTestOutputDirPath() / name;
  if (!path.has_extension()) {
    path += ".png";
  }

  ASSERT_TRUE(cv::imwrite(path, image)) << "Failed to save image to " << path.string();
}

void inspect(const cv::Mat &image, std::string title) {
  std::string testID = getCurrentTestID();
  if (title.empty()) {
    title = testID;
  } else {
    title = testID + " - " + title;
  }

#ifdef MANUAL_INSPECTION
  cv::imshow(title, image);
  cv::waitKey(0);
#endif
}
