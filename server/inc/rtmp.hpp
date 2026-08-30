#ifndef RTMP_H
#define RTMP_H

#include <arpa/inet.h>
#include <librtmp/rtmp.h>
#include <opencv2/core.hpp>
extern "C" { // Janky as hell
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libswscale/swscale.h>
}

#include <condition_variable>
#include <memory>
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
                      const std::string &cert = "",
                      const std::string &key = "");
  RTMPServer(const std::string &cert, const std::string &key)
      : RTMPServer(1935, "0.0.0.0", cert, key) {}

  ~RTMPServer();

  RTMPServer(const RTMPServer &) = delete;
  RTMPServer &operator=(const RTMPServer &) = delete;

  bool isRunning() const { return isActive; }

  /**
   * Receives the next video frame for the given stream name, if available.
   * Returns an empty optional if the stream doesn't exist or if there are no
   * new frames.
   */
  std::optional<cv::Mat> receiveStreamFrame(const std::string &name);

  /**
   * Returns the names of all currently active streams (i.e. active RTMP
   * connections that are currently publishing video data).
   */
  std::unordered_set<std::string> getActiveStreamNames() const;

  /**
   * Returns whether a stream with the given name is currently active.
   */
  bool isStreamActive(const std::string &name) const;

private:
  // initialization

  void initRTMPLogLevel();
  void initFFmpegLogLevel();
  bool startServer();

  // serving

  void acceptConnections();

  struct ClientInfo {
    int socketFd;
    char address[INET_ADDRSTRLEN];
  };

  void handleNewConnection(ClientInfo clientInfo);

  struct StreamInfo {
    int streamID = -1;

    std::string name = "";
    bool hasReceivedMetadata = false;
    // We only care about video metadata now, ignore audio stuff
    size_t width = 0, height = 0;
    double videoDataRate = 0.0, frameRate = 0.0;
    const AVCodec *codec = nullptr;
    const AVBitStreamFilter *bsf = nullptr;
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

  static cv::Mat avFrameToCvMat(const AVFrame *frame);

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

  mutable std::mutex streamMutex;
  int lastStreamID = 0;

  // RAII wrapper for AVCodecContext
  struct CodecContext {
    AVCodecContext *context = nullptr;
    AVBSFContext *bsfContext = nullptr;
    std::mutex mutex;

    explicit CodecContext(AVCodecContext *codecContext,
                          AVBSFContext *bsf = nullptr)
        : context(codecContext), bsfContext(bsf) {}

    ~CodecContext() {
      if (context) {
        avcodec_free_context(&context);
      }
    }
  };
  std::unordered_map<std::string, std::shared_ptr<CodecContext>> activeStreams;
};

#endif
