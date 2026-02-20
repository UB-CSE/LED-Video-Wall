#ifndef RTMP_H
#define RTMP_H

#include <librtmp/rtmp.h>
#include <thread>

class RTMPServer {
public:
  explicit RTMPServer(int port = 1935, const char *address = "0.0.0.0",
                      const char *cert = nullptr, const char *key = nullptr);

  ~RTMPServer();

  bool isRunning() const { return init_successful && state != State::STOPPED; }

  // TODO: Method to get frames from stream

private:
  // initialization

  void initRTMPLogLevel();
  bool startServer();

  // serving

  void serverThread();
  void serve(int client_sockfd);
  bool servePacket(RTMP *rtmp, RTMPPacket *packet);
  bool serveInvoke(RTMP *rtmp, RTMPPacket *packet, unsigned int offset);

  bool sendConnectResult(RTMP *rtmp, double txn);
  bool sendResultNumber(RTMP *rtmp, double txn, double id);
  bool sendPublish(RTMP *rtmp);

  // cleanup

  void stopServer();

  // utilities

  static void avReplace(AVal *src, const AVal *orig, const AVal *repl);

private:
  bool init_successful = false;

  int port;
  const char *address;
  void *ssl_ctx = nullptr;

  enum class State { ACCEPTING, RUNNING, STOPPING, STOPPED } state;
  std::thread server_thread;

  int sockfd = 0;
  int stream_id;
};

#endif
