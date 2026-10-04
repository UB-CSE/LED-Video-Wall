#include "tests-util.hpp"

#include "canvas.hpp"

// TODO: Add more tests for all the elements....

using namespace testing;

static const std::filesystem::path CanvasConfigPath =
    TestResourcesDir / "canvas-configs" / "input.yaml";

static const std::filesystem::path ParrotTestImagePath =
    TestImagesDir / "parrot.jpg";
static constexpr int ParrotTestImageWidth = 150;
static constexpr int ParrotTestImageHeight = 200;

static const std::filesystem::path RainbowTestImagePath =
    TestImagesDir / "rainbow.png";
static const std::filesystem::path ButterflyTestImagePath =
    TestImagesDir / "butterfly.jpg";

#define VERIFY_CV_SIZE(size, expectedWidth, expectedHeight)                    \
  ASSERT_THAT(size.width, Eq(expectedWidth));                                  \
  ASSERT_THAT(size.height, Eq(expectedHeight));

#define VERIFY_CV_POINT(point, expectedX, expectedY)                           \
  ASSERT_THAT(point.x, Eq(expectedX));                                         \
  ASSERT_THAT(point.y, Eq(expectedY));

TEST(ImageElement, Gamut) {
  auto elem = std::make_shared<ImageElement>("parrot", ParrotTestImagePath);
  ASSERT_THAT(elem->isLoaded(), IsTrue());

  cv::Mat elemMat = elem->getPixelMatrix();
  ASSERT_THAT(elemMat.size().width, Eq(ParrotTestImageWidth));
  ASSERT_THAT(elemMat.size().height, Eq(ParrotTestImageHeight));

  elem->setLocation(cv::Point(32, 32));
  VERIFY_CV_POINT(elem->getLocation(), 32, 32);

  inspect(elemMat, "Parrot Image - Start");
  saveImage(elemMat, "Parrot_Original");

  // Resize - Preserving aspect ratio (default behavior).

  elem->setSize(cv::Size(64, 64));
  elemMat = elem->getPixelMatrix();
  VERIFY_CV_SIZE(elemMat.size(), 48, 64);
  VERIFY_CV_POINT(elem->getLocation(), 32, 32);

  inspect(elemMat, "Parrot Image - 48x64");
  saveImage(elemMat, "Parrot_48x64");

  // Resize - Stretch

  elem->setPreserveAspectRatio(false);
  elem->setSize(cv::Size(64, 64));
  elemMat = elem->getPixelMatrix();
  VERIFY_CV_SIZE(elemMat.size(), 64, 64);
  VERIFY_CV_POINT(elem->getLocation(), 32, 32);

  inspect(elemMat, "Parrot Image - 64x64");
  saveImage(elemMat, "Parrot_64x64");

  // Rotate 45 (counter-clockwise)

  elem->setPreserveAspectRatio(true);
  elem->setRotation(45);
  elemMat = elem->getPixelMatrix();
  VERIFY_CV_SIZE(elemMat.size(), 80, 80);
  // The top-left corner position gets changed to preserve the center position
  // on the canvas.
  VERIFY_CV_POINT(elem->getLocation(), 16, 24);

  inspect(elemMat, "ImageElement: Parrot Image - 45 deg");
  saveImage(elemMat, "Parrot_45Deg");
}

TEST(Canvas, ImageElements) {
  auto elem1 = std::make_shared<ImageElement>("rainbow", RainbowTestImagePath);
  ASSERT_THAT(elem1->isLoaded(), IsTrue());
  elem1->setSize(cv::Size(64, 64));

  auto elem2 =
      std::make_shared<ImageElement>("butterfly", ButterflyTestImagePath);
  ASSERT_THAT(elem2->isLoaded(), IsTrue());
  elem2->setSize(cv::Size(32, 32));
  elem2->setRotation(45);
  elem2->setLocation(cv::Point(20, 20));

  const cv::Size canvasSize(64, 64);
  VirtualCanvas canvas(canvasSize);

  canvas.addElement(elem1);
  canvas.addElement(elem2);
  ASSERT_THAT(canvas.getElements(), ElementsAreArray({elem2, elem1}));

  cv::Mat canvasMat = canvas.getPixelMatrix();
  VERIFY_CV_SIZE(canvasMat.size(), 64, 64);
  inspect(canvasMat, "Canvas");
  saveImage(canvasMat, "Canvas");
}

TEST(Canvas, LoadAndSave) {
  std::filesystem::current_path(TestImagesDir);

  const cv::Size canvasSize(128, 128);
  VirtualCanvas canvas(canvasSize);

  RTMPServer rtmpServer;
  bool result = canvas.loadElementConfig(CanvasConfigPath, rtmpServer);
  ASSERT_THAT(result, IsTrue());

  ASSERT_THAT(canvas.getElementCount(), Eq(3));
  auto elements = canvas.getElements();
  auto elementIt = elements.begin();
  {
    auto elem2 = std::dynamic_pointer_cast<TextElement>(*elementIt);
    ASSERT_THAT(elem2, NotNull());
  }
  ++elementIt;
  {
    auto elem1 = std::dynamic_pointer_cast<VideoElement>(*elementIt);
    ASSERT_THAT(elem1, NotNull());
  }
  ++elementIt;
  {
    auto elem0 = std::dynamic_pointer_cast<ImageElement>(*elementIt);
    ASSERT_THAT(elem0, NotNull());
    ASSERT_THAT(elem0->isLoaded(), IsTrue());
  }

  cv::Mat canvasMat = canvas.getPixelMatrix();
  VERIFY_CV_SIZE(canvasMat.size(), 128, 128);
  inspect(canvasMat, "Canvas");
  saveImage(canvasMat, "Canvas");

  // Save

  const std::filesystem::path savePath = getTestOutputDirPath() / "input.yaml";
  canvas.saveElementConfig(savePath);

  ASSERT_THAT(std::filesystem::exists(savePath), IsTrue());
}
