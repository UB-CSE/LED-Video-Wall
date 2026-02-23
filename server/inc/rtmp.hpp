#ifndef RTMP_H
#define RTMP_H

#include <librtmp/rtmp.h>
#include <opencv2/core.hpp>
#include <thread>
#include <unordered_set>
#include <string>
#include <optional>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>

class RTMPServer {
public:
  explicit RTMPServer(int port = 1935, const char *address = "0.0.0.0",
                      const char *cert = nullptr, const char *key = nullptr);

  ~RTMPServer();

  bool isRunning() const { return isActive; }

#if 0 // Implement these soon
  std::unordered_set<std::string> getActiveStreamNames() const;
  bool isStreamActive(const std::string &stream_name) const;
  std::optional<cv::Mat> getStreamFrame(const std::string &stream_name) const;
#endif

private:
  // initialization

  void initRTMPLogLevel();
  bool startServer();

  // serving

  void acceptConnections();
  void handleNewConnection(int clientSocketFd);

  void handleClient(int clientSocketFd);
  bool handlePacket(RTMP *rtmp, RTMPPacket *packet, int* streamID);
  bool handleInvoke(RTMP *rtmp, RTMPPacket *packet, unsigned int offset, int* streamID);

  bool sendConnectResult(RTMP *rtmp, double txn);
  bool sendResultNumber(RTMP *rtmp, double txn, double id);
  bool sendPublish(RTMP *rtmp, int streamID);

  // cleanup

  void stopServer();

  // utilities

  static void avReplace(AVal *src, const AVal *orig, const AVal *repl);

private:
  bool wasInitSuccessful = false;
  bool isActive = false;

  int port;
  const char *address;
  void *sslContext = nullptr;

  int socketFd = 0;

  std::thread serverThread;
  std::vector<std::thread> workerThreads;
  void workerThreadFunc();

  std::queue<int> clientQueue;

  std::mutex queueMutex;
  std::condition_variable queueCondition;

  int lastStreamID = 0;
  std::mutex streamMutex;
};

#endif
