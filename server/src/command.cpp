#include "command.hpp"
#include "canvas.hpp"
#include "text-render.hpp"
#include <cctype>
#include <optional>
#include <readline/history.h>
#include <readline/readline.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/types.h>

#pragma region Command Argument Parsing

std::optional<std::string_view>
CommandParser::NextToken(std::string_view &inputStr) {
  const char *input = inputStr.data();
  if (!input) {
    return std::nullopt;
  }

  ssize_t startIndex = -1;
  char quote = false;

  ssize_t i = 0;
  char c;
  for (c = *input; c > 0 && i < static_cast<ssize_t>(inputStr.length());
       c = *(++input), i++) {
    if (startIndex >= 0) {
      // Closing quote or whitespace ends the token.
      if (c == quote || (!quote && std::isspace(c))) {
        quote = 0;
        break;
      }
    } else if (c == '\'' || c == '"') {
      // Enter quote - start of token.
      startIndex = i + 1;
      quote = c;
    } else if (!std::isspace(c)) {
      // Non-whitespace - start of token.
      startIndex = i;
    }
  }

  if (startIndex < 0 || quote) {
    // All whitespace or no closing quote.
    return std::nullopt;
  }

  std::string_view token = inputStr.substr(startIndex, i - startIndex);
  inputStr.remove_prefix(i + (c > 0 ? 1 : 0));
  return token;
}

bool CommandParser::ToArgs(std::string_view input,
                           std::string_view &parsedCommandName,
                           std::vector<std::string_view> &parsedArgs) {
  std::optional<std::string_view> tokenOpt = NextToken(input);
  if (!tokenOpt.has_value()) {
    return false;
  }

  parsedCommandName = tokenOpt.value();

  while (true) {
    tokenOpt = NextToken(input);
    if (!tokenOpt.has_value()) {
      break;
    }
    parsedArgs.push_back(tokenOpt.value());
  }
  return true;
}

#pragma endregion

#pragma region Command Execution

namespace {

/**
 * doCommand functions should return a json-encoded CommandResult or an empty
 * json if there is an invocation error.
 */

struct CommandResult {
  bool success = true;

  // Common problems
  bool badInvocation = false;
  bool noSuchElement = false;
};

struct NewElementCommandResult {
  CommandResult base;
  std::string uid;

  NewElementCommandResult(const std::string &uid)
      : base{.success = true}, uid(uid) {}
};

struct CanvasGetElementsCommandResult {
  CommandResult base;
  nlohmann::json elements;

  CanvasGetElementsCommandResult(const nlohmann::json &elements)
      : base{.success = true}, elements(elements) {}
};

CommandResult BadInvocationResult{.success = false, .badInvocation = true};
CommandResult NoSuchElementResult{.success = false, .noSuchElement = true};

void to_json(nlohmann::json &j, const CommandResult &result) {
  j = nlohmann::json{
      {"success", result.success},
      {"bad-invocation", result.badInvocation},
      {"no-such-element", result.noSuchElement},
  };
}

void to_json(nlohmann::json &j, const NewElementCommandResult &result) {
  j = result.base;
  j["id"] = result.uid;
}

void to_json(nlohmann::json &j, const CanvasGetElementsCommandResult &result) {
  j = result.base;
  j["canvas"] = result.elements;
}

nlohmann::json doCanvasLoad(VirtualCanvas &vCanvas, Controller &controller,
                            std::span<std::string_view> args,
                            std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 1) {
    return BadInvocationResult;
  }
  const std::filesystem::path filePath(args[0]);

  bool success = vCanvas.loadElementConfig(filePath);

  // Controller needs to initialize events for all the new elements
  controller.reinitializeCanvasEvents();

  return CommandResult{.success = success};
}

nlohmann::json doCanvasSave(VirtualCanvas &vCanvas, Controller &controller,
                            std::span<std::string_view> args,
                            std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 1) {
    return BadInvocationResult;
  }
  const std::filesystem::path filePath(args[0]);

  vCanvas.saveElementConfig(filePath);
  return CommandResult{.success = true};
}

nlohmann::json doCanvasClear(VirtualCanvas &vCanvas, Controller &controller,
                             std::span<std::string_view> args,
                             std::shared_ptr<spdlog::logger> logger) {
  if (!args.empty()) {
    return BadInvocationResult;
  }

  vCanvas.clearElements();
  return CommandResult{.success = true};
}

nlohmann::json doCanvasGetElements(VirtualCanvas &vCanvas,
                                   Controller &controller,
                                   std::span<std::string_view> args,
                                   std::shared_ptr<spdlog::logger> logger) {
  if (!args.empty()) {
    return BadInvocationResult;
  }
  nlohmann::json elementsJSON = vCanvas.elementsToJSON();
  return CanvasGetElementsCommandResult(elementsJSON);
}

nlohmann::json doElementRename(VirtualCanvas &vCanvas, Controller &controller,
                               std::span<std::string_view> args,
                               std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  const std::string name(args[1]);

  std::shared_ptr<Element> element = vCanvas.getElement(id);
  if (!element) {
    return NoSuchElementResult;
  }
  element->setName(name);
  return CommandResult{.success = true};
}

nlohmann::json doElementMove(VirtualCanvas &vCanvas, Controller &controller,
                             std::span<std::string_view> args,
                             std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 3) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  int x, y;
  try {
    x = std::stoi(std::string(args[1]));
    y = std::stoi(std::string(args[2]));
  } catch (...) {
    return BadInvocationResult;
  }

  std::shared_ptr<Element> element = vCanvas.getElement(id);
  if (!element) {
    return NoSuchElementResult;
  }
  element->setLocation(cv::Point(x, y));
  return CommandResult{.success = true};
}

nlohmann::json doElementRotate(VirtualCanvas &vCanvas, Controller &controller,
                               std::span<std::string_view> args,
                               std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  double degrees;
  try {
    degrees = std::stod(std::string(args[1]));
  } catch (...) {
    return BadInvocationResult;
  }

  std::shared_ptr<Element> element = vCanvas.getElement(id);
  if (!element) {
    return NoSuchElementResult;
  }
  element->setRotation(degrees);
  return CommandResult{.success = true};
}

nlohmann::json doElementResize(VirtualCanvas &vCanvas, Controller &controller,
                               std::span<std::string_view> args,
                               std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 3) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  unsigned x, y;
  try {
    x = std::stoul(std::string(args[1]));
    y = std::stoul(std::string(args[2]));
  } catch (...) {
    return BadInvocationResult;
  }

  std::shared_ptr<Element> element = vCanvas.getElement(id);
  if (!element) {
    return NoSuchElementResult;
  }
  element->setSize(cv::Size(x, y));
  return CommandResult{.success = true};
}

nlohmann::json doElementMoveUp(VirtualCanvas &vCanvas, Controller &controller,
                               std::span<std::string_view> args,
                               std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 1) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);

  bool success = vCanvas.moveElementUp(id);
  return CommandResult{.success = success};
}

nlohmann::json doElementMoveDown(VirtualCanvas &vCanvas, Controller &controller,
                                 std::span<std::string_view> args,
                                 std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 1) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);

  bool success = vCanvas.moveElementDown(id);
  return CommandResult{.success = success};
}

nlohmann::json doElementDelete(VirtualCanvas &vCanvas, Controller &controller,
                               std::span<std::string_view> args,
                               std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 1) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);

  bool success = vCanvas.removeElement(id);

  // The element's event should get removed.
  controller.reinitializeCanvasEvents();

  return CommandResult{.success = success, .noSuchElement = !success};
}

nlohmann::json
doElementSetPreserveAspectRatio(VirtualCanvas &vCanvas, Controller &controller,
                                std::span<std::string_view> args,
                                std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 2) {
    return BadInvocationResult;
  }

  const std::string id(args[0]);
  bool preserveAspectRatio;

  if (args[1] == "true" || args[1] == "1") {
    preserveAspectRatio = true;
  }
  if (args[1] == "false" || args[1] == "0") {
    preserveAspectRatio = false;
  } else {
    return BadInvocationResult;
  }

  std::shared_ptr<Element> element = vCanvas.getElement(id);
  if (!element) {
    return NoSuchElementResult;
  }
  element->setPreserveAspectRatio(preserveAspectRatio);
  return CommandResult{.success = true};
}

nlohmann::json doImageNew(VirtualCanvas &vCanvas, Controller &controller,
                          std::span<std::string_view> args,
                          std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 1) {
    return BadInvocationResult;
  }
  const std::string filepath(args[0]);

  std::string uid = Element::NewUID();
  auto element = std::make_shared<ImageElement>(uid, filepath);
  vCanvas.addElement(element);

  return NewElementCommandResult(uid);
}

nlohmann::json doImageSetFile(VirtualCanvas &vCanvas, Controller &controller,
                              std::span<std::string_view> args,
                              std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  const std::string filepath(args[1]);

  auto element =
      std::dynamic_pointer_cast<ImageElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->loadImageFile(filepath);
  return CommandResult{.success = true};
}

nlohmann::json doCarouselNew(VirtualCanvas &vCanvas, Controller &controller,
                             std::span<std::string_view> args,
                             std::shared_ptr<spdlog::logger> logger) {
  if (args.empty()) {
    return BadInvocationResult;
  }
  int frameRate;
  try {
    frameRate = std::stoi(std::string(args[0]));
  } catch (...) {
    return BadInvocationResult;
  }

  std::vector<std::string> filepaths;
  if (args.size() > 1) {
    filepaths = std::vector<std::string>(std::next(args.begin()), args.end());
  }

  std::string uid = Element::NewUID();
  auto element = std::make_shared<CarouselElement>(uid, filepaths, frameRate);
  vCanvas.addElement(element);

  return NewElementCommandResult(uid);
}

nlohmann::json doCarouselSetFiles(VirtualCanvas &vCanvas,
                                  Controller &controller,
                                  std::span<std::string_view> args,
                                  std::shared_ptr<spdlog::logger> logger) {
  if (args.empty()) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);

  auto element =
      std::dynamic_pointer_cast<CarouselElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  if (args.size() == 1) {
    element->loadImages({});
  } else {
    std::vector<std::string> filepaths(std::next(args.begin()), args.end());
    element->loadImages(filepaths);
  }
  return CommandResult{.success = true};
}

nlohmann::json doCarouselSetFrameRate(VirtualCanvas &vCanvas,
                                      Controller &controller,
                                      std::span<std::string_view> args,
                                      std::shared_ptr<spdlog::logger> logger) {
  if (args.size() < 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  int frameRate;
  try {
    frameRate = std::stoi(std::string(args[1]));
  } catch (...) {
    return BadInvocationResult;
  }

  auto element =
      std::dynamic_pointer_cast<CarouselElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->setFrameRate(frameRate);

  // The element's event period was changed.
  controller.reinitializeCanvasEvents();

  return CommandResult{.success = true};
}

nlohmann::json doVideoNew(VirtualCanvas &vCanvas, Controller &controller,
                          std::span<std::string_view> args,
                          std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 2) {
    return BadInvocationResult;
  }
  const std::string filepath(args[0]);
  int frameRate;
  try {
    frameRate = std::stoi(std::string(args[1]));
  } catch (...) {
    return BadInvocationResult;
  }

  std::string uid = Element::NewUID();
  auto element = std::make_shared<VideoElement>(uid, filepath, frameRate);
  vCanvas.addElement(element);

  return NewElementCommandResult(uid);
}

nlohmann::json doVideoSetFile(VirtualCanvas &vCanvas, Controller &controller,
                              std::span<std::string_view> args,
                              std::shared_ptr<spdlog::logger> logger) {
  if (args.size() < 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  const std::string filepath(args[1]);

  auto element =
      std::dynamic_pointer_cast<VideoElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->loadVideo(filepath);

  return CommandResult{.success = true};
}

nlohmann::json doVideoSetFrameRate(VirtualCanvas &vCanvas,
                                   Controller &controller,
                                   std::span<std::string_view> args,
                                   std::shared_ptr<spdlog::logger> logger) {
  if (args.size() < 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  int frameRate;
  try {
    frameRate = std::stoi(std::string(args[1]));
  } catch (...) {
    return BadInvocationResult;
  }

  auto element =
      std::dynamic_pointer_cast<VideoElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->setFrameRate(frameRate);

  // The element's event period was changed.
  controller.reinitializeCanvasEvents();

  return CommandResult{.success = true};
}

nlohmann::json doRTMPNew(VirtualCanvas &vCanvas, Controller &controller,
                         std::span<std::string_view> args,
                         std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 2) {
    return BadInvocationResult;
  }
  const std::string streamName(args[0]);
  int frameRate;
  try {
    frameRate = std::stoi(std::string(args[1]));
  } catch (...) {
    return BadInvocationResult;
  }

  std::string uid = Element::NewUID();
  auto element = std::make_shared<RTMPStreamElement>(
      uid, vCanvas.getRTMPServer(), streamName, frameRate);
  vCanvas.addElement(element);

  return NewElementCommandResult(uid);
}

nlohmann::json doRTMPSetStreamName(VirtualCanvas &vCanvas,
                                   Controller &controller,
                                   std::span<std::string_view> args,
                                   std::shared_ptr<spdlog::logger> logger) {
  if (args.size() < 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  const std::string streamName(args[1]);

  auto element =
      std::dynamic_pointer_cast<RTMPStreamElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->setStreamName(streamName);

  return CommandResult{.success = true};
}

nlohmann::json doRTMPSetFrameRate(VirtualCanvas &vCanvas,
                                  Controller &controller,
                                  std::span<std::string_view> args,
                                  std::shared_ptr<spdlog::logger> logger) {
  if (args.size() < 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  int frameRate;
  try {
    frameRate = std::stoi(std::string(args[1]));
  } catch (...) {
    return BadInvocationResult;
  }

  auto element =
      std::dynamic_pointer_cast<RTMPStreamElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->setFrameRate(frameRate);

  // The element's event period was changed.
  controller.reinitializeCanvasEvents();

  return CommandResult{.success = true};
}

nlohmann::json doWebBrowserNew(VirtualCanvas &vCanvas, Controller &controller,
                               std::span<std::string_view> args,
                               std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 4) {
    return BadInvocationResult;
  }
  const std::string url(args[0]);
  int frameRate;
  unsigned viewSizeWidth;
  unsigned viewSizeHeight;
  try {
    frameRate = std::stoi(std::string(args[1]));
    viewSizeWidth = std::stoul(std::string(args[2]));
    viewSizeHeight = std::stoul(std::string(args[3]));
  } catch (...) {
    return BadInvocationResult;
  }

  std::string uid = Element::NewUID();
  auto element = std::make_shared<WebBrowserElement>(
      uid, url, frameRate, cv::Size(viewSizeWidth, viewSizeHeight));
  vCanvas.addElement(element);

  return NewElementCommandResult(uid);
}

nlohmann::json doWebBrowserSetURL(VirtualCanvas &vCanvas,
                                  Controller &controller,
                                  std::span<std::string_view> args,
                                  std::shared_ptr<spdlog::logger> logger) {
  if (args.size() < 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  const std::string url(args[1]);

  auto element =
      std::dynamic_pointer_cast<WebBrowserElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->setURL(url);

  return CommandResult{.success = true};
}

nlohmann::json doWebBrowserSetViewSize(VirtualCanvas &vCanvas,
                                       Controller &controller,
                                       std::span<std::string_view> args,
                                       std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 3) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  unsigned x, y;
  try {
    x = std::stoul(std::string(args[1]));
    y = std::stoul(std::string(args[2]));
  } catch (...) {
    return BadInvocationResult;
  }

  auto element =
      std::dynamic_pointer_cast<WebBrowserElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->setViewSize(cv::Size(x, y));

  return CommandResult{.success = true};
}

nlohmann::json
doWebBrowserSetFrameRate(VirtualCanvas &vCanvas, Controller &controller,
                         std::span<std::string_view> args,
                         std::shared_ptr<spdlog::logger> logger) {
  if (args.size() < 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  int frameRate;
  try {
    frameRate = std::stoi(std::string(args[1]));
  } catch (...) {
    return BadInvocationResult;
  }

  auto element =
      std::dynamic_pointer_cast<WebBrowserElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->setFrameRate(frameRate);

  // The element's event period was changed.
  controller.reinitializeCanvasEvents();

  return CommandResult{.success = true};
}

nlohmann::json doWebBrowserSetCookie(VirtualCanvas &vCanvas,
                                     Controller &controller,
                                     std::span<std::string_view> args,
                                     std::shared_ptr<spdlog::logger> logger) {
  if (args.size() < 7 || args.size() > 8) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  const std::string name(args[1]);
  const std::string value(args[2]);
  const std::string domain(args[3]);
  const std::string path(args[4]);

  auto getBoolean = [](std::string_view arg) {
    if (arg == "true" || arg == "1") {
      return true;
    }
    if (arg == "false" || arg == "0") {
      return false;
    }

    throw std::runtime_error("not a boolean");
  };

  bool secure = false;
  bool httpOnly = false;

  try {
    secure = getBoolean(args[5]);
    httpOnly = getBoolean(args[6]);
  } catch (...) {
    return BadInvocationResult;
  }

  cef_cookie_same_site_t sameSite = CEF_COOKIE_SAME_SITE_UNSPECIFIED;
  if (args.size() == 8) {
    const auto sameSiteStr = args[7];
    if (sameSiteStr == "None") {
      sameSite = CEF_COOKIE_SAME_SITE_NO_RESTRICTION;
    } else if (sameSiteStr == "Lax") {
      sameSite = CEF_COOKIE_SAME_SITE_LAX_MODE;
    } else if (sameSiteStr == "Strict") {
      sameSite = CEF_COOKIE_SAME_SITE_STRICT_MODE;
    } else if (!sameSiteStr.empty()) {
      return BadInvocationResult;
    }
  }

  auto element =
      std::dynamic_pointer_cast<WebBrowserElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->setCookie(name, value, domain, path, secure, httpOnly, sameSite);

  return CommandResult{.success = true};
}

nlohmann::json
doWebBrowserDeleteCookie(VirtualCanvas &vCanvas, Controller &controller,
                         std::span<std::string_view> args,
                         std::shared_ptr<spdlog::logger> logger) {
  if (args.size() < 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  const std::string cookieName(args[1]);

  auto element =
      std::dynamic_pointer_cast<WebBrowserElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->deleteCookie(cookieName);

  return CommandResult{.success = true};
}

nlohmann::json doTextNew(VirtualCanvas &vCanvas, Controller &controller,
                         std::span<std::string_view> args,
                         std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 6) {
    return BadInvocationResult;
  }
  const std::string content(args[0]);
  const std::string fontPath(args[1]);
  int fontSize;
  unsigned r, g, b;
  try {
    fontSize = std::stoi(std::string(args[2]));
    r = std::stoul(std::string(args[3]));
    g = std::stoul(std::string(args[4]));
    b = std::stoul(std::string(args[5]));
  } catch (...) {
    return BadInvocationResult;
  }
  if (r > 255 || g > 255 || b > 255) {
    return BadInvocationResult;
  }

  std::string uid = Element::NewUID();
  auto element = std::make_shared<TextElement>(uid, content, fontPath, fontSize,
                                               cv::Scalar(b, g, r));
  vCanvas.addElement(element);

  return NewElementCommandResult(uid);
}

nlohmann::json doTextSetContent(VirtualCanvas &vCanvas, Controller &controller,
                                std::span<std::string_view> args,
                                std::shared_ptr<spdlog::logger> logger) {
  if (args.size() < 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  const std::string content(args[1]);

  auto element = std::dynamic_pointer_cast<TextElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->setText(content);

  return CommandResult{.success = true};
}

nlohmann::json doTextSetFont(VirtualCanvas &vCanvas, Controller &controller,
                             std::span<std::string_view> args,
                             std::shared_ptr<spdlog::logger> logger) {
  if (args.size() < 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  const std::string filepath(args[1]);

  auto element = std::dynamic_pointer_cast<TextElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->setFont(filepath);

  return CommandResult{.success = true};
}

nlohmann::json doTextSetColor(VirtualCanvas &vCanvas, Controller &controller,
                              std::span<std::string_view> args,
                              std::shared_ptr<spdlog::logger> logger) {
  if (args.size() < 4) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);

  unsigned r, g, b;
  try {
    r = std::stoul(std::string(args[1]));
    g = std::stoul(std::string(args[2]));
    b = std::stoul(std::string(args[3]));
  } catch (...) {
    return BadInvocationResult;
  }
  if (r > 255 || g > 255 || b > 255) {
    return BadInvocationResult;
  }

  auto element = std::dynamic_pointer_cast<TextElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->setColor(cv::Scalar(b, g, r));

  return CommandResult{.success = true};
}

using DoCommandFunc = nlohmann::json(VirtualCanvas &vCanvas,
                                     Controller &controller,
                                     std::span<std::string_view> args,
                                     std::shared_ptr<spdlog::logger> logger);

struct CommandInfo {
  DoCommandFunc *doCommandFunc;
  std::string usage;
};

std::map<std::string, CommandInfo> commands{
    // Canvas
    {
        "canvas-load",
        CommandInfo{
            .doCommandFunc = doCanvasLoad,
            .usage = "<filepath>",
        },
    },
    {
        "canvas-save",
        CommandInfo{
            .doCommandFunc = doCanvasSave,
            .usage = "<filepath>",
        },
    },
    {
        "canvas-clear",
        CommandInfo{
            .doCommandFunc = doCanvasClear,
            .usage = "",
        },
    },
    {
        "canvas-get-elements",
        CommandInfo{
            .doCommandFunc = doCanvasGetElements,
            .usage = "",
        },
    },
    // Common element properties
    {
        "element-rename",
        CommandInfo{
            .doCommandFunc = doElementRename,
            .usage = "<id> <name>",
        },
    },
    {
        "element-move",
        CommandInfo{
            .doCommandFunc = doElementMove,
            .usage = "<id> <x> <y>",
        },
    },
    {
        "element-rotate",
        CommandInfo{
            .doCommandFunc = doElementRotate,
            .usage = "<id> <degrees>",
        },
    },
    {
        "element-resize",
        CommandInfo{
            .doCommandFunc = doElementResize,
            .usage = "<id> <width> <height>",
        },
    },
    {
        "element-move-up",
        CommandInfo{
            .doCommandFunc = doElementMoveUp,
            .usage = "<id>",
        },
    },
    {
        "element-move-down",
        CommandInfo{
            .doCommandFunc = doElementMoveDown,
            .usage = "<id>",
        },
    },
    {
        "element-delete",
        CommandInfo{
            .doCommandFunc = doElementDelete,
            .usage = "<id>",
        },
    },
    {"element-set-preserve-aspect-ratio",
     CommandInfo{
         .doCommandFunc = doElementSetPreserveAspectRatio,
         .usage = "<id> <true|false>",
     }},
    // Image element properties
    {
        "image-new",
        CommandInfo{
            .doCommandFunc = doImageNew,
            .usage = "<filepath>",
        },
    },
    {
        "image-set-file",
        CommandInfo{
            .doCommandFunc = doImageSetFile,
            .usage = "<id> <filepath>",
        },
    },
    // Carousel element properties
    {
        "carousel-new",
        CommandInfo{
            .doCommandFunc = doCarouselNew,
            .usage = "<framerate> [filepaths...]",
        },
    },
    {
        "carousel-set-files",
        CommandInfo{
            .doCommandFunc = doCarouselSetFiles,
            .usage = "<id> [filepaths...]",
        },
    },
    {
        "carousel-set-framerate",
        CommandInfo{
            .doCommandFunc = doCarouselSetFrameRate,
            .usage = "<id> <framerate>",
        },
    },
    // Video element properties
    {
        "video-new",
        CommandInfo{
            .doCommandFunc = doVideoNew,
            .usage = "<filepath> <framerate>",
        },
    },
    {
        "video-set-file",
        CommandInfo{
            .doCommandFunc = doVideoSetFile,
            .usage = "<id> <filepath>",
        },
    },
    {
        "video-set-framerate",
        CommandInfo{
            .doCommandFunc = doVideoSetFrameRate,
            .usage = "<id> <framerate>",
        },
    },
    // RTMP stream element properties
    {
        "rtmp-new",
        CommandInfo{
            .doCommandFunc = doRTMPNew,
            .usage = "<stream-name> <framerate>",
        },
    },
    {
        "rtmp-set-stream-name",
        CommandInfo{
            .doCommandFunc = doRTMPSetStreamName,
            .usage = "<id> <stream-name>",
        },
    },
    {
        "rtmp-set-framerate",
        CommandInfo{
            .doCommandFunc = doRTMPSetFrameRate,
            .usage = "<id> <framerate>",
        },
    },
    // Web browser element properties
    {
        "web-browser-new",
        CommandInfo{
            .doCommandFunc = doWebBrowserNew,
            .usage = "<url> <framerate> <view-width> <view-height>",
        },
    },
    {
        "web-browser-set-url",
        CommandInfo{
            .doCommandFunc = doWebBrowserSetURL,
            .usage = "<id> <url>",
        },
    },
    {
        "web-browser-set-view-size",
        CommandInfo{
            .doCommandFunc = doWebBrowserSetViewSize,
            .usage = "<id> <width> <height>",
        },
    },
    {
        "web-browser-set-framerate",
        CommandInfo{
            .doCommandFunc = doWebBrowserSetFrameRate,
            .usage = "<id> <framerate>",
        },
    },
    {
        "web-browser-set-cookie",
        CommandInfo{
            .doCommandFunc = doWebBrowserSetCookie,
            .usage = "<id> <name> <value> <domain> <path> <secure> <httpOnly> "
                     "[sameSite]",
        },
    },
    {
        "web-browser-delete-cookie",
        CommandInfo{
            .doCommandFunc = doWebBrowserDeleteCookie,
            .usage = "<id> <cookie-name>",
        },
    },
    // Text element properties
    {
        "text-new",
        CommandInfo{
            .doCommandFunc = doTextNew,
            .usage = "<content> <font-path> <font-size> <r> <g> <b>",
        },
    },
    {
        "text-set-content",
        CommandInfo{
            .doCommandFunc = doTextSetContent,
            .usage = "<id> <content>",
        },
    },
    {
        "text-set-font",
        CommandInfo{
            .doCommandFunc = doTextSetFont,
            .usage = "<id> <filepath>",
        },
    },
    {
        "text-set-color",
        CommandInfo{
            .doCommandFunc = doTextSetColor,
            .usage = "<id> <r> <g> <b>",
        },
    },

};

void doHelp(std::shared_ptr<spdlog::logger> logger) {
  std::stringstream ss;

  ss << "usage:\nAvailable commands:\n";
  ss << "\thelp\n\tquit\n\tpause\n\tresume";

  for (const auto &[name, info] : commands) {
    ss << "\n\t" << name << " " << info.usage;
  }

  logger->info(ss.str());
}

} // namespace

nlohmann::json ProcessCommand(VirtualCanvas &vCanvas, Controller &controller,
                              const std::string &line, bool &isPaused,
                              bool &isRunning,
                              std::shared_ptr<spdlog::logger> logger) {
  spdlog::info("[Command] ProcessCommand: `{}'", line);

  std::string_view commandName;
  std::vector<std::string_view> args;
  if (!CommandParser::ToArgs(line, commandName, args)) {
    logger->error("failed to parse arguments: `{}'", line);
    return BadInvocationResult;
  }

  // TODO: Commands to edit each element and get canvas data.

  if (commandName == "help") {
    doHelp(logger);
  } else if (commandName == "quit" || commandName == "exit") {
    isRunning = false;
  } else if (commandName == "pause") {
    isPaused = true;
  } else if (commandName == "resume") {
    isPaused = false;
  } else if (const auto it = commands.find(std::string(commandName));
             it != commands.end()) {
    auto &[doCommandFunc, usage] = it->second;
    nlohmann::json result = doCommandFunc(vCanvas, controller, args, logger);
    if (result["bad-invocation"].get<bool>()) {
      logger->error("{}: invalid invocation, usage: {} {}", commandName,
                    commandName, usage);
    } else if (result["no-such-element"].get<bool>()) {
      logger->error("{}: no element with given id", commandName);
    }
    return result;
  } else {
    logger->error("no such command `{}'", commandName);
    return BadInvocationResult;
  }

  return CommandResult{.success = true};
}

#pragma endregion

#pragma region Prompt Command Source

PromptCommandSource::PromptCommandSource() {
  m_cmdOutputLogger = spdlog::default_logger()->clone("LEDVW-Command-Prompt");
  m_cmdOutputLogger->sinks().push_back(
      std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

  rl_catch_signals = false;

  rl_event_hook = []() -> int {
    if (!get().m_isRunning) {
      rl_done = true;
    }
    return 0;
  };

  m_thread = std::jthread(&PromptCommandSource::threadFunc, this);
}

PromptCommandSource::~PromptCommandSource() { m_isRunning = false; }

void PromptCommandSource::setPromptName(std::string_view name) {
  std::lock_guard lock(m_mutex);
  m_promptName = name;
}

std::string PromptCommandSource::consumeLatestCommand() {
  std::lock_guard lock(m_mutex);
  if (m_cmdQueue.empty()) {
    return "";
  }
  std::string cmd = m_cmdQueue.front();
  m_cmdQueue.pop();
  return cmd;
}

void PromptCommandSource::threadFunc() {
  while (m_isRunning) {
    if (!m_isActive) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    std::string prompt;
    {
      std::lock_guard lock(m_mutex);
      prompt = m_promptName;
    }
    prompt += "> ";

    char *line = readline(prompt.c_str());
    if (!m_isRunning) {
      break;
    }

    if (!line || *line == '\0') {
      continue;
    }

    bool isAllWhitespace = true;
    for (char *c = line; *c != '\0'; c++) {
      isAllWhitespace = isAllWhitespace && std::isspace(*c);
    }
    if (isAllWhitespace) {
      continue;
    }

    add_history(line);

    std::string lineString(line);
    free(line);

    {
      std::lock_guard lock(m_mutex);
      m_cmdQueue.push(lineString);
      m_isActive = false;
    }

    if (lineString == "exit" || lineString == "quit") {
      //  break;
    }
  }
}

#pragma endregion

#pragma region Unix Socket Command Source

UnixSocketCommandSource::UnixSocketCommandSource(
    const std::filesystem::path &path)
    : m_channel(path) {
  m_cmdOutputLogger = spdlog::default_logger()->clone("LEDVW-Command");

  m_lastHeartbeatTime = std::chrono::steady_clock::now();
}

void UnixSocketCommandSource::process() {
  m_channel.process();

  auto now = std::chrono::steady_clock::now();
  if (m_channel.isConnected()) {
    auto [success, receiveBuffer] = m_channel.receiveMessage();
    if (success && !receiveBuffer.empty()) {
      std::string commandString(
          reinterpret_cast<const char *>(receiveBuffer.data()),
          receiveBuffer.size());
      m_cmdQueue.push(commandString);
    }

    // Send heartbeat messages every second to verify that we're still
    // connected.
    auto elapsed = now - m_lastHeartbeatTime;
    if (elapsed > std::chrono::milliseconds(1000)) {
      m_lastHeartbeatTime = now;
      m_channel.sendMessage({});
    }
  } else {
    m_lastHeartbeatTime = now;
  }
}

std::string UnixSocketCommandSource::consumeLatestCommand() {
  if (m_cmdQueue.empty()) {
    return "";
  }

  std::string cmd = m_cmdQueue.front();
  m_cmdQueue.pop();
  return cmd;
}

void UnixSocketCommandSource::handleResponse(const nlohmann::json &response) {
  std::string responseString = response.dump();
  std::span responseBuffer(
      reinterpret_cast<const uint8_t *>(responseString.data()),
      responseString.size());
  m_channel.sendMessage(responseBuffer);
}

#pragma endregion
