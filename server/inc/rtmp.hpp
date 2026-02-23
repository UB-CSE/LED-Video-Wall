#ifndef RTMP_H
#define RTMP_H

#include <arpa/inet.h>
#include <librtmp/rtmp.h>
#include <opencv2/core.hpp>
extern "C" { // Janky as hell
#include <libavcodec/avcodec.h>
}

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

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

  struct ClientInfo {
    int socketFd;
    // const char* address;
    char address[INET_ADDRSTRLEN];
  };

  void handleNewConnection(ClientInfo clientInfo);

  struct StreamInfo {
    int streamID = -1;

    bool hasReceivedMetadata = false;
    // We only care about video metadata now.
    size_t width = 0, height = 0;
    double videoDataRate = 0.0, frameRate = 0.0;
    const AVCodec *codec = nullptr;
    AVCodecContext *codecContext = nullptr;

    ~StreamInfo();
    void reset();
  };

  void handleClient(ClientInfo clientInfo);
  bool handlePacket(RTMP *rtmp, RTMPPacket *packet, StreamInfo &streamInfo,
                    const ClientInfo &clientInfo);
  bool handleChangeChunkSize(RTMP *rtmp, RTMPPacket *packet);
  bool handleInvoke(RTMP *rtmp, RTMPPacket *packet, size_t offset,
                    StreamInfo &streamInfo, const ClientInfo &clientInfo);
  bool handleMetadata(RTMP *rtmp, RTMPPacket *packet, StreamInfo &streamInfo,
                      const ClientInfo &clientInfo);
  bool handleVideoPacket(RTMP *rtmp, RTMPPacket *packet, StreamInfo &streamInfo,
                         const ClientInfo &clientInfo);

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

  std::queue<ClientInfo> clientQueue;

  std::mutex queueMutex;
  std::condition_variable queueCondition;

  int lastStreamID = 0;
  std::mutex streamMutex;
};

#endif
