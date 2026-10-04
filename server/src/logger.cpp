#include "logger.hpp"

extern "C" {
// Janky as hell
#include <libavutil/log.h>
}
#include <ctime>
#include <format>
#include <librtmp/log.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

static std::shared_ptr<spdlog::logger> s_ffmpegLogger;
static std::shared_ptr<spdlog::logger> s_rtmpLogger;

namespace {
std::string printfFormatToString(const char *fmt, va_list vl) {
  va_list copy;
  va_copy(copy, vl);

  int size = std::vsnprintf(nullptr, 0, fmt, copy);
  va_end(copy);

  if (size < 0) {
    return "";
  }

  std::string message(static_cast<std::size_t>(size), '\0');

  va_copy(copy, vl);
  std::vsnprintf(message.data(), message.size() + 1, fmt, copy);
  va_end(copy);

  if (!message.empty()) {
    if (message.back() == '\n') {
      message.pop_back();
    }
  }

  return message;
}

void rtmpLogCallback(int level, const char *fmt, va_list vl) {
  std::string message = printfFormatToString(fmt, vl);
  if (message.empty()) {
    return;
  }

  switch (level) {
  case RTMP_LOGCRIT:
    s_rtmpLogger->critical("{}", message);
    break;
  case RTMP_LOGERROR:
    s_rtmpLogger->error("{}", message);
    break;
  case RTMP_LOGWARNING:
    s_rtmpLogger->warn("{}", message);
    break;
  case RTMP_LOGINFO:
    s_rtmpLogger->info("{}", message);
    break;
  case RTMP_LOGDEBUG:
    s_rtmpLogger->debug("{}", message);
    break;
  default:
    break;
  }
}

void captureLibRTMPLogs() {
  if (!s_rtmpLogger) {
    s_rtmpLogger = spdlog::default_logger()->clone("LibRTMP");
  }

  RTMP_LogSetCallback(rtmpLogCallback);
}

void ffmpegLogCallback(void *, int level, const char *fmt, va_list vl) {
  std::string message = printfFormatToString(fmt, vl);
  if (message.empty()) {
    return;
  }

  switch (level) {
  case AV_LOG_PANIC:
    s_ffmpegLogger->critical("{}", message);
    break;
  case AV_LOG_FATAL:
  case AV_LOG_ERROR:
    s_ffmpegLogger->error("{}", message);
    break;
  case AV_LOG_WARNING:
    s_ffmpegLogger->warn("{}", message);
    break;
  case AV_LOG_INFO:
    s_ffmpegLogger->info("{}", message);
    break;
  case AV_LOG_VERBOSE:
    s_ffmpegLogger->debug("{}", message);
    break;
  default:
    break;
  }
}

void captureFFMPEGLogs() {
  if (!s_ffmpegLogger) {
    s_ffmpegLogger = spdlog::default_logger()->clone("FFMPEG");
  }

  av_log_set_callback(ffmpegLogCallback);
}

} // namespace

std::filesystem::path
InitializeLogging(bool withStdout, const std::filesystem::path &logsDirectory,
                  size_t maxLogFiles) {
  static constexpr std::string LoggerName = "LEDVW";

  std::filesystem::path logFilePath;

  std::vector<spdlog::sink_ptr> sinks;

  if (withStdout) {
    auto stdoutLogSink =
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sinks.push_back(stdoutLogSink);
  }
  if (!logsDirectory.empty()) {
    CleanUpLogsDirectory(maxLogFiles, logsDirectory);

    logFilePath = MakeLogFilePath(logsDirectory);
    auto fileLogSink =
        std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, true);
    sinks.push_back(fileLogSink);
  }

  std::shared_ptr<spdlog::logger> logger =
      std::make_shared<spdlog::logger>(LoggerName, sinks.begin(), sinks.end());
  spdlog::set_default_logger(logger);

  captureLibRTMPLogs();
  captureFFMPEGLogs();

  return logFilePath;
}

std::filesystem::path
MakeLogFilePath(const std::filesystem::path &logsDirectory,
                const std::string &name /*= ""*/) {
  time_t now = std::time(nullptr);
  std::tm *localTime = std::localtime(&now);

  std::string timestamp = std::format(
      "{:04}-{:02}-{:02}_{:02}-{:02}-{:02}", localTime->tm_year + 1900,
      localTime->tm_mon + 1, localTime->tm_mday, localTime->tm_hour,
      localTime->tm_min, localTime->tm_sec);

  std::string filename;
  if (!name.empty()) {
    filename = name + '_';
  }
  filename += timestamp + ".log";

  return logsDirectory / filename;
}

namespace {

struct LogFileTimestamp {
  int year, month, day, hour, minute, second;

  bool operator<(const LogFileTimestamp &other) const {
    if (year != other.year)
      return year < other.year;

    if (month != other.month)
      return month < other.month;

    if (day != other.day)
      return day < other.day;

    if (hour != other.hour)
      return hour < other.hour;

    if (minute != other.minute)
      return minute < other.minute;

    if (second != other.second)
      return second < other.second;

    return false;
  }
};

struct LogFile {
  LogFileTimestamp timestamp;
  std::filesystem::path path;

  bool operator<(const LogFile &other) const {
    return timestamp < other.timestamp;
  }
};

} // namespace

void CleanUpLogsDirectory(size_t maxFiles, const std::filesystem::path &logsDir,
                          const std::string &name) {
  if (!std::filesystem::exists(logsDir)) {
    return;
  }

  std::vector<LogFile> logFiles;

  for (auto const &dirEntry : std::filesystem::directory_iterator(logsDir)) {
    if (!dirEntry.is_regular_file())
      continue;

    std::filesystem::path path = dirEntry.path();

    if (path.extension() != ".log")
      continue;

    std::string stem = path.stem().string();

    /**
     * Make sure the name fits the Name_YYYY-MM-DD_HH-MM-SS.log format. If it
     * runs into an error at any point, just skip the file.
     */
    if (stem.size() < 19)
      continue;

    std::string_view timestampString = stem;

    if (!name.empty()) {
      if (!timestampString.starts_with(name + "_")) {
        continue;
      }

      timestampString.remove_prefix(name.length() + 1);
    }

    if (timestampString.size() > 19) {
      continue;
    }

    LogFileTimestamp timestamp;

    try {
      const int year = std::stoi(std::string(timestampString.substr(0, 4)));
      timestampString.remove_prefix(5);

      const int month = std::stoi(std::string(timestampString.substr(0, 2)));
      timestampString.remove_prefix(3);

      const int day = std::stoi(std::string(timestampString.substr(0, 2)));
      timestampString.remove_prefix(3);

      const int hour = std::stoi(std::string(timestampString.substr(0, 2)));
      timestampString.remove_prefix(3);

      const int minute = std::stoi(std::string(timestampString.substr(0, 2)));
      timestampString.remove_prefix(3);

      const int second = std::stoi(std::string(timestampString));

      timestamp = LogFileTimestamp{.year = year,
                                   .month = month,
                                   .day = day,
                                   .hour = hour,
                                   .minute = minute,
                                   .second = second};
    }

    catch (...) {
      continue;
    }

    logFiles.push_back(LogFile{.timestamp = timestamp, .path = path});
  }

  std::sort(logFiles.begin(), logFiles.end());

  if (logFiles.size() > maxFiles) {
    for (size_t i = 0; i < logFiles.size() - maxFiles; i++) {
      std::filesystem::remove(logFiles.at(i).path);
    }
  }
}