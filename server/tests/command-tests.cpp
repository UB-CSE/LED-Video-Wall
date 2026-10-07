#include <tests-util.hpp>

#include "command.hpp"
#include <format>

using namespace testing;

#pragma region CommandParser Tests

TEST(CommandParser, Empty) {
  std::string_view input;

  std::optional<std::string_view> token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());

  input = "    ";
  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());
}

TEST(CommandParser, Basic) {
  std::string_view input = "mycommand arg1 arg2  arg3  ";

  std::optional<std::string_view> token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("mycommand"));
  ASSERT_THAT(input, StrEq("arg1 arg2  arg3  "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("arg1"));
  ASSERT_THAT(input, StrEq("arg2  arg3  "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("arg2"));
  ASSERT_THAT(input, StrEq(" arg3  "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("arg3"));
  ASSERT_THAT(input, StrEq(" "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());
}

TEST(CommandParser, SingleQuotes) {
  std::string_view input = "mycommand arg1\t'long \"argument\"' ";

  std::optional<std::string_view> token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("mycommand"));
  ASSERT_THAT(input, StrEq("arg1\t'long \"argument\"' "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("arg1"));
  ASSERT_THAT(input, StrEq("'long \"argument\"' "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("long \"argument\""));
  ASSERT_THAT(input, StrEq(" "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());
}

TEST(CommandParser, SingleQuotesEmpty) {
  std::string_view input = "''";

  std::optional<std::string_view> token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq(""));
  ASSERT_THAT(input, StrEq(""));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());
}

TEST(CommandParser, DoubleQuotes) {
  std::string_view input = "mycommand arg1\t\"long 'argument'\" ";

  std::optional<std::string_view> token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("mycommand"));
  ASSERT_THAT(input, StrEq("arg1\t\"long 'argument'\" "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("arg1"));
  ASSERT_THAT(input, StrEq("\"long 'argument'\" "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("long 'argument'"));
  ASSERT_THAT(input, StrEq(" "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());
}

TEST(CommandParser, DoubleQuotesEmpty) {
  std::string_view input = "\"\"";

  std::optional<std::string_view> token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq(""));
  ASSERT_THAT(input, StrEq(""));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());
}

TEST(CommandParser, BadQuotes) {
  {
    std::string_view input = "mycommand arg1\t\"long 'argument' ";

    std::optional<std::string_view> token = CommandParser::NextToken(input);
    ASSERT_THAT(token.has_value(), IsTrue());
    ASSERT_THAT(token.value(), StrEq("mycommand"));
    ASSERT_THAT(input, StrEq("arg1\t\"long 'argument' "));

    token = CommandParser::NextToken(input);
    ASSERT_THAT(token.has_value(), IsTrue());
    ASSERT_THAT(token.value(), StrEq("arg1"));
    ASSERT_THAT(input, StrEq("\"long 'argument' "));

    token = CommandParser::NextToken(input);
    ASSERT_THAT(token.has_value(), IsFalse());
  }

  {
    std::string_view input = "'";

    std::optional<std::string_view> token = CommandParser::NextToken(input);
    ASSERT_THAT(token.has_value(), IsFalse());
  }
}

TEST(CommandParser, ToArgs) {
  std::string_view input = "mycommand arg1\t\"long 'argument'\" arg3\r\n";
  std::string_view command;
  std::vector<std::string_view> args;

  bool result = CommandParser::ToArgs(input, command, args);
  ASSERT_THAT(result, IsTrue());
  ASSERT_THAT(command, StrEq("mycommand"));
  ASSERT_THAT(args, ElementsAre("arg1", "long 'argument'", "arg3"));
}

TEST(CommandParser, ToArgsNoCommand) {
  std::string_view input = "   ";
  std::string_view command;
  std::vector<std::string_view> args;

  bool result = CommandParser::ToArgs(input, command, args);
  ASSERT_THAT(result, IsFalse());
}

#pragma endregion

#pragma region Command Tests

namespace {

class Commands : public Test {
protected:
  const cv::Size canvasSize{128, 128};
  RTMPServer rtmpServer;
  VirtualCanvas canvas{canvasSize, rtmpServer};
  Controller controller{canvas, nullptr, 40000000};

  nlohmann::json responseJSON;

public:
  Commands() = default;
  ~Commands() override = default;

  static inline const std::filesystem::path CanvasConfigPath =
      TestResourcesDir / "canvas-configs" / "input.yaml";

  void validateCommonResponseJSON(bool badInvocation = false,
                                  bool noSuchElement = false) {
    ASSERT_THAT(responseJSON, Not(IsEmpty()));

    ASSERT_THAT(responseJSON.contains("bad-invocation"), IsTrue());
    ASSERT_THAT(responseJSON["bad-invocation"].is_boolean(), IsTrue());
    ASSERT_THAT(responseJSON["bad-invocation"].get<bool>(), Eq(badInvocation));

    ASSERT_THAT(responseJSON.contains("no-such-element"), IsTrue());
    ASSERT_THAT(responseJSON["no-such-element"].is_boolean(), IsTrue());
    ASSERT_THAT(responseJSON["no-such-element"].get<bool>(), Eq(noSuchElement));

    const bool expectedSuccess = !badInvocation && !noSuchElement;

    ASSERT_THAT(responseJSON.contains("success"), IsTrue());
    ASSERT_THAT(responseJSON["success"].is_boolean(), IsTrue());
    ASSERT_THAT(responseJSON["success"].get<bool>(), Eq(expectedSuccess));
  }

  void doCommand(const std::string &line, bool *isPausedPtr = nullptr,
                 bool *isRunningPtr = nullptr) {
    bool isPaused = isPausedPtr ? *isPausedPtr : false;
    bool isRunning = isRunningPtr ? *isRunningPtr : true;

    responseJSON = ProcessCommand(canvas, controller, line, isPaused, isRunning,
                                  spdlog::default_logger());

    if (isPausedPtr) {
      *isPausedPtr = isPaused;
    } else {
      ASSERT_THAT(isPaused, IsFalse());
    }

    if (isRunningPtr) {
      *isRunningPtr = isRunning;
    } else {
      ASSERT_THAT(isPaused, IsFalse());
    }
  }
};

} // namespace

TEST_F(Commands, Load) {
  const std::string commandLine =
      std::format(R"(load "{}")", CanvasConfigPath.string());

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(canvas.getElementCount(), Eq(3));
  // See the test "Canvas.LoadAndSave" for full load testing.
}

TEST_F(Commands, Save) {
  ASSERT_THAT(canvas.loadElementConfig(CanvasConfigPath), IsTrue());

  const std::filesystem::path saveFilePath =
      getTestOutputDirPath() / "input.yaml";
  ASSERT_THAT(std::filesystem::exists(saveFilePath), IsFalse());

  const std::string commandLine =
      std::format(R"(save "{}")", saveFilePath.string());

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(std::filesystem::exists(saveFilePath), IsTrue());
}

TEST_F(Commands, ElementRename) {
  auto elem = std::make_shared<ImageElement>("parrot", ParrotTestImagePath);
  elem->setName("Cool Parrot");

  canvas.addElement(elem);

  const std::string newName = "Awesome Parrot";

  const std::string commandLine =
      std::format(R"(element-rename parrot "{}")", newName);

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(elem->getName(), StrEq(newName));
}

TEST_F(Commands, ElementMove) {
  auto elem = std::make_shared<ImageElement>("parrot", ParrotTestImagePath);
  elem->setLocation(cv::Point(10, 15));

  canvas.addElement(elem);

  cv::Point newLocation(25, 30);

  const std::string commandLine =
      std::format("element-move parrot {} {}", newLocation.x, newLocation.y);

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(elem->getLocation().x, Eq(newLocation.x));
  ASSERT_THAT(elem->getLocation().y, Eq(newLocation.y));
}

TEST_F(Commands, ElementRotate) {
  auto elem = std::make_shared<ImageElement>("parrot", ParrotTestImagePath);
  elem->setRotation(15);

  canvas.addElement(elem);

  double newRotation = 25.7;

  const std::string commandLine =
      std::format("element-rotate parrot {}", newRotation);

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(elem->getRotation(), DoubleEq(newRotation));
}

TEST_F(Commands, ElementMoveUp) {
  auto elem1 = std::make_shared<ImageElement>("parrot", ParrotTestImagePath);
  auto elem2 =
      std::make_shared<ImageElement>("butterfly", ButterflyTestImagePath);

  canvas.addElement(elem1);
  canvas.addElement(elem2);

  ASSERT_THAT(canvas.getElements(), ElementsAre(elem2, elem1));

  const std::string commandLine = std::format("element-move-up parrot");

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(canvas.getElements(), ElementsAre(elem1, elem2));
}

TEST_F(Commands, ElementMoveDown) {
  auto elem1 = std::make_shared<ImageElement>("parrot", ParrotTestImagePath);
  auto elem2 =
      std::make_shared<ImageElement>("butterfly", ButterflyTestImagePath);

  canvas.addElement(elem1);
  canvas.addElement(elem2);

  ASSERT_THAT(canvas.getElements(), ElementsAre(elem2, elem1));

  const std::string commandLine = std::format("element-move-down butterfly");

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(canvas.getElements(), ElementsAre(elem1, elem2));
}

TEST_F(Commands, ElementDelete) {
  auto elem1 = std::make_shared<ImageElement>("parrot", ParrotTestImagePath);
  auto elem2 =
      std::make_shared<ImageElement>("butterfly", ButterflyTestImagePath);

  canvas.addElement(elem1);
  canvas.addElement(elem2);

  ASSERT_THAT(canvas.getElements(), ElementsAre(elem2, elem1));

  const std::string commandLine = std::format("element-delete butterfly");

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(canvas.getElements(), ElementsAre(elem1));
}

TEST_F(Commands, ImageNew) {
  const std::string commandLine =
      std::format(R"(image-new "{}")", ButterflyTestImagePath.string());

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(responseJSON.contains("id"), IsTrue());
  ASSERT_THAT(responseJSON["id"].is_string(), IsTrue());
  const auto uid = responseJSON["id"].get<std::string>();

  auto element =
      std::dynamic_pointer_cast<ImageElement>(canvas.getElement(uid));
  ASSERT_THAT(element, NotNull());

  ASSERT_THAT(element->getImageFilePath(), StrEq(ButterflyTestImagePath));
}

TEST_F(Commands, ImageSetFile) {
  auto elem = std::make_shared<ImageElement>("parrot", ParrotTestImagePath);
  canvas.addElement(elem);

  const std::string commandLine = std::format(R"(image-set-file parrot "{}")",
                                              ButterflyTestImagePath.string());

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(elem->getImageFilePath(), StrEq(ButterflyTestImagePath.string()));
}

TEST_F(Commands, CarouselNew) {
  const std::vector filepaths = {
      ParrotTestImagePath.string(),
      ButterflyTestImagePath.string(),
  };

  const std::string commandLine =
      std::format(R"(carousel-new 3 "{}" "{}")", filepaths[0], filepaths[1]);

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(responseJSON.contains("id"), IsTrue());
  ASSERT_THAT(responseJSON["id"].is_string(), IsTrue());
  const auto uid = responseJSON["id"].get<std::string>();

  auto element =
      std::dynamic_pointer_cast<CarouselElement>(canvas.getElement(uid));
  ASSERT_THAT(element, NotNull());

  ASSERT_THAT(element->getImageFilePaths(), ElementsAreArray(filepaths));
  ASSERT_THAT(element->getFrameRate(), Eq(3));
}

TEST_F(Commands, CarouselSetFiles) {
  const std::vector originalFilepaths = {
      ParrotTestImagePath.string(),
      ButterflyTestImagePath.string(),
  };
  auto elem =
      std::make_shared<CarouselElement>("carousel", originalFilepaths, 1);
  canvas.addElement(elem);

  const std::vector newFilePaths = {
      ButterflyTestImagePath.string(),
      RainbowTestImagePath.string(),
  };

  const std::string commandLine =
      std::format(R"(carousel-set-files carousel "{}" "{}")", newFilePaths[0],
                  newFilePaths[1]);

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(elem->getImageFilePaths(), ElementsAreArray(newFilePaths));
}

TEST_F(Commands, CarouselSetFrameRate) {
  const std::vector filePaths = {
      ParrotTestImagePath.string(),
      ButterflyTestImagePath.string(),
  };
  auto elem = std::make_shared<CarouselElement>("carousel", filePaths, 1);
  canvas.addElement(elem);

  constexpr int newFrameRate = 2;

  const std::string commandLine =
      std::format("carousel-set-framerate carousel {}", newFrameRate);

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(elem->getFrameRate(), Eq(newFrameRate));
}

TEST_F(Commands, VideoSetFile) {
  auto elem = std::make_shared<VideoElement>("alan", AlanTestVideoPath, 30);
  canvas.addElement(elem);

  const std::string commandLine =
      std::format(R"(video-set-file alan "{}")", ConwayTestVideoPath.string());

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(elem->getVideoFilePath(), StrEq(ConwayTestVideoPath.string()));
}

TEST_F(Commands, VideoNew) {
  const std::string commandLine =
      std::format(R"(video-new "{}" 30)", AlanTestVideoPath.string());

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(responseJSON.contains("id"), IsTrue());
  ASSERT_THAT(responseJSON["id"].is_string(), IsTrue());
  const auto uid = responseJSON["id"].get<std::string>();

  auto element =
      std::dynamic_pointer_cast<VideoElement>(canvas.getElement(uid));
  ASSERT_THAT(element, NotNull());

  ASSERT_THAT(element->getVideoFilePath(), StrEq(AlanTestVideoPath.string()));
  ASSERT_THAT(element->getFrameRate(), Eq(30));
}

TEST_F(Commands, VideoSetFrameRate) {
  auto elem = std::make_shared<VideoElement>("alan", AlanTestVideoPath, 30);
  canvas.addElement(elem);

  constexpr int newFrameRate = 20;

  const std::string commandLine =
      std::format("video-set-framerate alan {}", newFrameRate);

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(elem->getFrameRate(), Eq(newFrameRate));
}

TEST_F(Commands, RTMPNew) {
  const std::string streamName = "My Stream";

  const std::string commandLine =
      std::format(R"(rtmp-new "{}" 30)", streamName);

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(responseJSON.contains("id"), IsTrue());
  ASSERT_THAT(responseJSON["id"].is_string(), IsTrue());
  const auto uid = responseJSON["id"].get<std::string>();

  auto element =
      std::dynamic_pointer_cast<RTMPStreamElement>(canvas.getElement(uid));
  ASSERT_THAT(element, NotNull());

  ASSERT_THAT(element->getStreamName(), StrEq(streamName));
  ASSERT_THAT(element->getFrameRate(), Eq(30));
}

TEST_F(Commands, RTMPSetStreamName) {
  auto elem =
      std::make_shared<RTMPStreamElement>("rtmp", rtmpServer, "mystream", 30);
  canvas.addElement(elem);

  const std::string newStreamName = "New Stream";

  const std::string commandLine =
      std::format(R"(rtmp-set-stream-name rtmp "{}")", newStreamName);

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(elem->getStreamName(), StrEq(newStreamName));
}

TEST_F(Commands, RTMPSetFrameRate) {
  auto elem =
      std::make_shared<RTMPStreamElement>("rtmp", rtmpServer, "mystream", 30);
  canvas.addElement(elem);

  constexpr int newFrameRate = 20;

  const std::string commandLine =
      std::format("rtmp-set-framerate rtmp {}", newFrameRate);

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(elem->getFrameRate(), Eq(newFrameRate));
}

// Unfortunately we cannot initialize CEF for unit tests so no web browser
// command tests.

TEST_F(Commands, TextNew) {
  const std::string content = "My Text";

  const std::string commandLine =
      std::format(R"(text-new "{}" "{}" 24 255 128 200)", content,
                  RobotoFontPath.string());

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(responseJSON.contains("id"), IsTrue());
  ASSERT_THAT(responseJSON["id"].is_string(), IsTrue());
  const auto uid = responseJSON["id"].get<std::string>();

  auto element =
      std::dynamic_pointer_cast<TextElement>(canvas.getElement(uid));
  ASSERT_THAT(element, NotNull());

  ASSERT_THAT(element->getText(), StrEq(content));
  ASSERT_THAT(element->getFontPath(), StrEq(RobotoFontPath.string()));
  ASSERT_THAT(element->getFontSize(), Eq(24));
  ASSERT_THAT(element->getColor()[0], Eq(200)); // B
  ASSERT_THAT(element->getColor()[1], Eq(128)); // G
  ASSERT_THAT(element->getColor()[2], Eq(255)); // R
}

TEST_F(Commands, TextSetContent) {
  auto elem = std::make_shared<TextElement>("elem", "Hello!", RobotoFontPath,
                                            24, cv::Scalar(0, 0, 255));
  canvas.addElement(elem);

  const std::string newContent = "Does this work??";

  const std::string commandLine =
      std::format(R"(text-set-content elem "{}")", newContent);

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(elem->getText(), StrEq(newContent));
}

TEST_F(Commands, TextSetFont) {
  auto elem = std::make_shared<TextElement>("elem", "Hello!", RobotoFontPath,
                                            24, cv::Scalar(0, 0, 255));
  canvas.addElement(elem);

  const std::string commandLine =
      std::format(R"(text-set-font elem "{}")", LobsterFontPath.string());

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(elem->getFontPath(), StrEq(LobsterFontPath.string()));
}

TEST_F(Commands, TextSetColor) {
  auto elem = std::make_shared<TextElement>("elem", "Hello!", RobotoFontPath,
                                            24, cv::Scalar(0, 0, 255));
  canvas.addElement(elem);

  const std::string commandLine =
      std::format("text-set-color elem 100 200 50"); // R, G, B

  doCommand(commandLine);
  validateCommonResponseJSON();

  ASSERT_THAT(elem->getColor()[0], Eq(50));  // B
  ASSERT_THAT(elem->getColor()[1], Eq(200)); // G
  ASSERT_THAT(elem->getColor()[2], Eq(100)); // R
}

#pragma endregion