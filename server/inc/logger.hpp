#pragma once

#include <filesystem>
#include <spdlog/spdlog.h>
#include <string>

static constexpr size_t DefaultMaxLogFiles = 25;

/**
 * Initialize the default spdlog logger to output to stdout, a file, or both.
 * Also redirects logs from LibRTMP and FFMPEG to the same location.
 *
 * @param withStdout If false, the logger will not output to stdout.
 * @param logsDirectory If not empty, a log file will be created in the given
 *                      directory.
 * @param maxLogFiles Maximum number of log files allowed in logsDirectory
 *                    before cleanup needs to happen
 *
 * @return Path to the log file, or an empty string if not file logging
 */
std::filesystem::path
InitializeLogging(bool withStdout = true,
                  const std::filesystem::path &logsDirectory = "",
                  size_t maxLogFiles = DefaultMaxLogFiles);

/**
 * Returns a path to a log file in the specified logs directory.
 * The filename will be in the format: Name_YYYY-MM-DD_HH-MM-SS.log
 *
 * @param logsDirectory Directory where the log file will be created.
 * @param name Optional name prefix for the log file. If provided, it will be
 *             prepended to the filename.
 *
 * @return A path to the log file.
 */
std::filesystem::path
MakeLogFilePath(const std::filesystem::path &logsDirectory,
                const std::string &name = "");

/**
 * Removes old log files from the specified logs directory.
 * Log files must be in the format: Name_YYYY-MM-DD_HH-MM-SS.log
 *
 * Oldest files get removed first.
 *
 * @param maxFiles Maximum number of log files allowed to keep in the directory.
 * @param logsDir Directory to clean up.
 * @param name Name prefix of the log files to cleanup.
 */
void CleanUpLogsDirectory(size_t maxFiles, const std::filesystem::path &logsDir,
                          const std::string &name = "");