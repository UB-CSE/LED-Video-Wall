#include "rtmp.hpp"

#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <librtmp/log.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <spdlog/spdlog.h>
#include <string_view>
#include <sys/socket.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_WORKER_THREADS 5

static constexpr AVal av_app = AVC("app");
static constexpr AVal av_connect = AVC("connect");
static constexpr AVal av_flashVer = AVC("flashVer");
static constexpr AVal av_swfUrl = AVC("swfUrl");
static constexpr AVal av_pageUrl = AVC("pageUrl");
static constexpr AVal av_tcUrl = AVC("tcUrl");
static constexpr AVal av_capabilities = AVC("capabilities");
static constexpr AVal av_audioCodecs = AVC("audioCodecs");
static constexpr AVal av_videoCodecs = AVC("videoCodecs");
static constexpr AVal av_objectEncoding = AVC("objectEncoding");
static constexpr AVal av_result = AVC("_result");
static constexpr AVal av_createStream = AVC("createStream");
static constexpr AVal av_deleteStream = AVC("deleteStream");
static constexpr AVal av_setDataFrame = AVC("@setDataFrame");
static constexpr AVal av_onMetaData = AVC("onMetaData");
static constexpr AVal av_getStreamLength = AVC("getStreamLength");
static constexpr AVal av_fmsVer = AVC("fmsVer");
static constexpr AVal av_mode = AVC("mode");
static constexpr AVal av_level = AVC("level");
static constexpr AVal av_code = AVC("code");
static constexpr AVal av_description = AVC("description");
// static constexpr AVal av_secureToken = AVC("secureToken");
static constexpr AVal av_onStatus = AVC("onStatus");
static constexpr AVal av_publish = AVC("publish");

#define STR2AVAL(av, str)                                                      \
  do {                                                                         \
    av.av_val = str;                                                           \
    av.av_len = strlen(av.av_val);                                             \
  } while (0)

enum RTMPVideoCodecID {
  RTMP_VIDEOCODEC_H263 = 2,      // H.263 video
  RTMP_VIDEOCODEC_SCREEN = 3,    // Screen sharing video
  RTMP_VIDEOCODEC_VP6 = 4,       // On2 VP6 video
  RTMP_VIDEOCODEC_VP6_ALPHA = 5, // On2 VP6 with alpha
  RTMP_VIDEOCODEC_SCREEN2 = 6,   // Screen sharing video (version 2)
  RTMP_VIDEOCODEC_H264 = 7,      // H.264 video
  RTMP_VIDEOCODEC_HEVC = 12,     // H.265 video (non-standard)
  RTMP_VIDEOCODEC_AV1 = 13       // AV1 video (non-standard)
};

enum RTMPAudioCodecID {
  RTMP_AUDIOCODEC_MP3 = 2,  // MP3 audio
  RTMP_AUDIOCODEC_AAC = 10, // AAC audio
};

enum RTMPVideoFrameType {
  RTMP_VIDEOFRAME_KEYFRAME = 1,        // Key frame
  RTMP_VIDEOFRAME_INTERFRAME = 2,      // Inter frame
  RTMP_VIDEOFRAME_DISPOSABLEFRAME = 3, // Disposable inter frame
  RTMP_VIDEOFRAME_SEQUENCEHEADER = 4   // AVC/H.264 sequence header
};

enum RTMPAudioFrameType {
  RTMP_AUDIOFRAME_SEQUENCEHEADER = 0, // AAC sequence header
  RTMP_AUDIOFRAME_RAWDATA = 1         // AAC raw data
};

RTMPServer::RTMPServer(int port /*= 1935*/, const char *address /*= "0.0.0.0"*/,
                       const std::string &cert /*= ""*/,
                       const std::string &key /*= ""*/)
    : port(port), address(address) {
  if (!cert.empty() && !key.empty()) {
    sslContext = RTMP_TLS_AllocServerContext(cert.c_str(), key.c_str());
    if (!sslContext) {
      spdlog::error("[RTMPServer] failed to initialize TLS context with cert "
                    "`{}' and key `{}'",
                    cert, key);
      return;
    }
  }

  wasInitSuccessful = startServer();

  if (!wasInitSuccessful) {
    spdlog::error("[RTMPServer] failed to start server");
  }
}

RTMPServer::~RTMPServer() {
  if (wasInitSuccessful) {
    stopServer();
  }

  if (sslContext) {
    RTMP_TLS_FreeServerContext(sslContext);
  }

  spdlog::info("[RTMPServer] stopped (port {})", port);
}

bool RTMPServer::receiveStreamFrame(const std::string &name, cv::Mat &output) {
  if (!wasInitSuccessful || !isActive) {
    return false;
  }

  std::shared_ptr<CodecContext> codecContext;
  {
    std::lock_guard<std::mutex> lk(streamMutex);
    auto it = activeStreams.find(name);
    if (it == activeStreams.end()) {
      return false;
    }
    codecContext = it->second;
  }

  AVFrame *frame = av_frame_alloc();
  int ret;
  {
    std::lock_guard<std::mutex> lk(codecContext->mutex);
    ret = avcodec_receive_frame(codecContext->context, frame);
  }

  if (ret != 0) {
    av_frame_free(&frame);
    return false;
  }

  avFrameToCvMat(frame, output);

  av_frame_free(&frame);
  return true;
}

std::unordered_set<std::string> RTMPServer::getActiveStreamNames() const {
  if (!wasInitSuccessful || !isActive) {
    return {};
  }

  std::unordered_set<std::string> streamNames;

  {
    std::lock_guard<std::mutex> lk(streamMutex);
    for (const auto &entry : activeStreams) {
      streamNames.insert(entry.first);
    }
  }

  return streamNames;
}

bool RTMPServer::isStreamActive(const std::string &name) const {
  if (!wasInitSuccessful || !isActive) {
    return false;
  }

  std::lock_guard<std::mutex> lk(streamMutex);
  return activeStreams.find(name) != activeStreams.end();
}

bool RTMPServer::startServer() {
  socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socketFd < 0) {
    spdlog::error("[RTMPServer] failed to create socket: {}", strerror(errno));
    return false;
  }

  do {
    int tmp = 1;
    setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<char *>(&tmp), sizeof(tmp));

    int flags = fcntl(socketFd, F_GETFL, 0);
    if (flags < 0) {
      spdlog::error("[RTMPServer] failed to get socket flags: {}",
                    strerror(errno));
      break;
    }
    if (fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) < 0) {
      spdlog::error("[RTMPServer] failed to set socket to non-blocking: {}",
                    strerror(errno));
      break;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(address);
    addr.sin_port = htons(port);
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

    if (bind(socketFd, reinterpret_cast<struct sockaddr *>(&addr),
             sizeof(struct sockaddr_in)) < 0) {
      spdlog::error("[RTMPServer] failed to bind socket: {}", strerror(errno));
      break;
    }

    if (listen(socketFd, 10) < 0) {
      spdlog::error("[RTMPServer] failed to listen on socket: {}",
                    strerror(errno));
      break;
    }

    serverThread = std::thread(std::bind(&RTMPServer::acceptConnections, this));

    for (size_t i = 0; i < NUM_WORKER_THREADS; i++) {
      workerThreads.emplace_back(
          std::bind(&RTMPServer::workerThreadFunc, this));
    }

    spdlog::info("[RTMPServer] successfully started on port {}", port);
    return true;

  } while (false);

  // Failure
  close(socketFd);
  return false;
}

void RTMPServer::stopServer() {
  {
    std::lock_guard<std::mutex> lk(queueMutex);
    clientQueue = std::queue<ClientInfo>();
    isActive = false;
  }
  queueCondition.notify_all();

  for (std::thread &worker : workerThreads) {
    worker.join();
  }
  serverThread.join();

  if (close(socketFd) < 0) {
    spdlog::error("[RTMPServer] failed to close socket: {}", strerror(errno));
  }
}

void RTMPServer::acceptConnections() {
  isActive = true;

  while (isActive) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int clientSocketFd =
        accept(socketFd, reinterpret_cast<struct sockaddr *>(&addr), &addrlen);

    if (clientSocketFd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No incoming connection.
      } else {
        spdlog::error("[RTMPServer] failed to accept connection: {}",
                      strerror(errno));
      }
      continue;
    }

    /**
     * Remove non-blocking flag from client socket just in case the client
     * inherits it (seems to be a macOS thing).
     */
    int flags = fcntl(clientSocketFd, F_GETFL, 0);
    if (flags >= 0) {
      fcntl(clientSocketFd, F_SETFL, flags & ~O_NONBLOCK);
    }

    ClientInfo clientInfo;
    clientInfo.socketFd = clientSocketFd;

    if (!inet_ntop(AF_INET, &addr.sin_addr, clientInfo.address,
                   sizeof(clientInfo.address))) {
      spdlog::error("[RTMPServer] failed to get client address: {}",
                    strerror(errno));
      close(clientSocketFd);
      continue;
    }

    spdlog::info("[RTMPServer] {}: connected", clientInfo.address);

    handleNewConnection(clientInfo);
  }
}

void RTMPServer::handleNewConnection(ClientInfo clientInfo) {
  {
    std::lock_guard<std::mutex> lk(queueMutex);
    clientQueue.push(clientInfo);
  }
  queueCondition.notify_one();
}

void RTMPServer::workerThreadFunc() {
  while (isActive) {
    ClientInfo clientInfo;

    {
      std::unique_lock<std::mutex> lk(queueMutex);
      queueCondition.wait(lk,
                          [this] { return !isActive || !clientQueue.empty(); });
      if (!isActive) {
        break;
      }

      clientInfo = clientQueue.front();
      clientQueue.pop();
    }

    handleClient(clientInfo);
  }
}

void RTMPServer::handleClient(ClientInfo clientInfo) {
  const auto &[clientSocketFd, clientAddress] = clientInfo;

  // Timeout for http requests
  struct timeval tv;
  memset(&tv, 0, sizeof(struct timeval));
  tv.tv_sec = 5;

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(clientSocketFd, &fds);

  StreamInfo streamInfo;

  do {
    if (select(clientSocketFd + 1, &fds, nullptr, nullptr, &tv) <= 0) {
      spdlog::error("[RTMPServer] {}: timeout waiting for client request: {}",
                    clientAddress, strerror(errno));
      break;
    }

    RTMP *rtmp = RTMP_Alloc();
    do {
      RTMP_Init(rtmp);
      rtmp->m_sb.sb_socket = clientSocketFd;
      if (sslContext && !RTMP_TLS_Accept(rtmp, sslContext)) {
        spdlog::error("[RTMPServer] {}: TLS handshake failed", clientAddress);
        break;
      }
      if (!RTMP_Serve(rtmp)) {
        spdlog::error("[RTMPServer] {}: handshake failed", clientAddress);
        break;
      }

      RTMPPacket packet{};
      while (RTMP_IsConnected(rtmp) && RTMP_ReadPacket(rtmp, &packet)) {
        if (!RTMPPacket_IsReady(&packet)) {
          continue;
        }
        bool result = handlePacket(rtmp, &packet, streamInfo, clientInfo);

        RTMPPacket_Free(&packet);
        if (!result) {
          break;
        }
      }
    } while (false);

    RTMP_Close(rtmp);
    // Should probably be done by RTMP_Close() ...
    rtmp->Link.playpath.av_val = nullptr;
    rtmp->Link.tcUrl.av_val = nullptr;
    rtmp->Link.swfUrl.av_val = nullptr;
    rtmp->Link.pageUrl.av_val = nullptr;
    rtmp->Link.app.av_val = nullptr;
    rtmp->Link.flashVer.av_val = nullptr;
    if (rtmp->Link.extras.o_num > 0) {
      delete[] rtmp->Link.extras.o_props;
      rtmp->Link.extras.o_num = 0;
    }
    if (rtmp->Link.usherToken.av_val) {
      delete rtmp->Link.usherToken.av_val;
      rtmp->Link.usherToken.av_val = nullptr;
    }
    RTMP_Free(rtmp);
  } while (false);

  close(clientSocketFd);
  spdlog::info("[RTMPServer] {}: disconnected", clientAddress);

  if (streamInfo.streamID != -1) {
    std::lock_guard<std::mutex> lk(streamMutex);
    activeStreams.erase(streamInfo.name);
  }
}

bool RTMPServer::handlePacket(RTMP *r, RTMPPacket *packet,
                              StreamInfo &streamInfo,
                              const ClientInfo &clientInfo) {
  RTMP_Log(RTMP_LOGDEBUG, "received packet type %02X, size %u bytes",
           packet->m_packetType, packet->m_nBodySize);

  switch (packet->m_packetType) {
  case RTMP_PACKET_TYPE_CHUNK_SIZE:
    return handleChangeChunkSize(r, packet);
  case RTMP_PACKET_TYPE_BYTES_READ_REPORT:
    break;
  case RTMP_PACKET_TYPE_CONTROL:
    break;
  case RTMP_PACKET_TYPE_SERVER_BW:
    break;
  case RTMP_PACKET_TYPE_CLIENT_BW:
    break;
  case RTMP_PACKET_TYPE_AUDIO:
    break;
  case RTMP_PACKET_TYPE_VIDEO:
    return handleVideoPacket(r, packet, streamInfo, clientInfo);
  case RTMP_PACKET_TYPE_FLEX_STREAM_SEND:
    break;
  case RTMP_PACKET_TYPE_FLEX_SHARED_OBJECT:
    break;
  case RTMP_PACKET_TYPE_FLEX_MESSAGE:
    RTMP_Log(RTMP_LOGDEBUG, "flex message, size %u bytes, not fully supported",
             packet->m_nBodySize);
    return handleInvoke(r, packet, 1, streamInfo, clientInfo);
  case RTMP_PACKET_TYPE_INFO:
    return handleMetadata(r, packet, streamInfo, clientInfo);
  case RTMP_PACKET_TYPE_SHARED_OBJECT:
    break;
  case RTMP_PACKET_TYPE_INVOKE:
    RTMP_Log(RTMP_LOGDEBUG, "received: invoke %u bytes", packet->m_nBodySize);
    return handleInvoke(r, packet, 0, streamInfo, clientInfo);
  case RTMP_PACKET_TYPE_FLASH_VIDEO:
    break;
  default:
    RTMP_Log(RTMP_LOGDEBUG, "unknown packet type received: 0x%02x",
             packet->m_packetType);
    break;
  }

  return true;
}

bool RTMPServer::handleChangeChunkSize(RTMP *r, RTMPPacket *packet) {
  if (packet->m_nBodySize < 4) {
    return false;
  }
  r->m_inChunkSize = AMF_DecodeInt32(packet->m_body);
  return true;
}

bool RTMPServer::handleInvoke(RTMP *r, RTMPPacket *packet, size_t offset,
                              StreamInfo &streamInfo,
                              const ClientInfo &clientInfo) {
  const char *body = packet->m_body + offset;
  size_t bodySize = packet->m_nBodySize - offset;

  if (body[0] != 0x02) {
    spdlog::error(
        "[RTMPServer] {}: sanity failed. no string method in invoke packet",
        clientInfo.address);
    return false;
  }

  AMFObject obj;
  if (AMF_Decode(&obj, body, bodySize, false) < 0) {
    spdlog::error("[RTMPServer] {}: error decoding invoke packet",
                  clientInfo.address);
    return false;
  }

  AMF_Dump(&obj);
  AVal method;
  AMFProp_GetString(AMF_GetProp(&obj, nullptr, 0), &method);
  double txn = AMFProp_GetNumber(AMF_GetProp(&obj, nullptr, 1));
  RTMP_Log(RTMP_LOGDEBUG, "client invoking <%s>", method.av_val);

  bool success = false;
  do {
    if (AVMATCH(&method, &av_connect)) {
      AMFObject cobj;
      AVal pname, pval;

      AMFProp_GetObject(AMF_GetProp(&obj, nullptr, 2), &cobj);
      for (int i = 0; i < cobj.o_num; i++) {
        pname = cobj.o_props[i].p_name;
        pval.av_val = nullptr;
        pval.av_len = 0;
        if (cobj.o_props[i].p_type == AMF_STRING)
          pval = cobj.o_props[i].p_vu.p_aval;
        if (AVMATCH(&pname, &av_app)) {
          r->Link.app = pval;
          pval.av_val = nullptr;
          if (!r->Link.app.av_val) {
            r->Link.app.av_val = "";
          }
          streamInfo.name = std::string(r->Link.app.av_val, r->Link.app.av_len);
        } else if (AVMATCH(&pname, &av_flashVer)) {
          r->Link.flashVer = pval;
          pval.av_val = nullptr;
        } else if (AVMATCH(&pname, &av_swfUrl)) {
          r->Link.swfUrl = pval;
          pval.av_val = nullptr;
        } else if (AVMATCH(&pname, &av_tcUrl)) {
          r->Link.tcUrl = pval;
          pval.av_val = nullptr;
        } else if (AVMATCH(&pname, &av_pageUrl)) {
          r->Link.pageUrl = pval;
          pval.av_val = nullptr;
        } else if (AVMATCH(&pname, &av_audioCodecs)) {
          r->m_fAudioCodecs = cobj.o_props[i].p_vu.p_number;
        } else if (AVMATCH(&pname, &av_videoCodecs)) {
          r->m_fVideoCodecs = cobj.o_props[i].p_vu.p_number;
        } else if (AVMATCH(&pname, &av_objectEncoding)) {
          r->m_fEncoding = cobj.o_props[i].p_vu.p_number;
        }
      }
      if (obj.o_num > 3) {
        int i = obj.o_num - 3;
        r->Link.extras.o_num = i;
        r->Link.extras.o_props = new AMFObjectProperty[i];
        memcpy(r->Link.extras.o_props, obj.o_props + 3,
               i * sizeof(AMFObjectProperty));
        obj.o_num = 3;
      }
      sendConnectResult(r, txn);
    } else if (AVMATCH(&method, &av_createStream)) {
      if (streamInfo.streamID < 0) {
        streamInfo.streamID = -1;
        streamInfo.hasReceivedMetadata = false;

        std::lock_guard<std::mutex> lk(streamMutex);
        if (activeStreams.find(streamInfo.name) != activeStreams.end()) {
          spdlog::warn(
              "[RTMPServer] {}: stream with name {} already exists, ignoring",
              clientInfo.address, streamInfo.name);
          break;
        }
        streamInfo.streamID = ++lastStreamID;
      } else {
        spdlog::warn("[RTMPServer] {}: received createStream while stream "
                     "already exists, ignoring",
                     clientInfo.address);
      }
      spdlog::info("[RTMPServer] {}: created stream {} (\"{}\")",
                   clientInfo.address, streamInfo.streamID, streamInfo.name);
      sendResultNumber(r, txn, streamInfo.streamID);
    } else if (AVMATCH(&method, &av_deleteStream)) {
      if (streamInfo.streamID < 0) {
        spdlog::warn("[RTMPServer] {}: received deleteStream before "
                     "createStream, ignoring",
                     clientInfo.address);
      }
      spdlog::info("[RTMPServer] {}: deleted stream {} (\"{}\")",
                   clientInfo.address, streamInfo.streamID, streamInfo.name);
      {
        std::lock_guard<std::mutex> lk(streamMutex);
        activeStreams.erase(streamInfo.name);
      }
      streamInfo = StreamInfo{};
    } else if (AVMATCH(&method, &av_getStreamLength)) {
      sendResultNumber(r, txn, 10.0);
    } else if (AVMATCH(&method, &av_publish)) {
      if (streamInfo.streamID < 0) {
        spdlog::warn(
            "[RTMPServer] {}: received publish before createStream, ignoring",
            clientInfo.address);
        break;
      }
      sendPublish(r, streamInfo.streamID);
    }
    success = true;

  } while (false);

  AMF_Reset(&obj);
  return success;
}

bool RTMPServer::handleMetadata(RTMP *r, RTMPPacket *packet,
                                StreamInfo &streamInfo,
                                const ClientInfo &clientInfo) {
  const char *body = packet->m_body;
  uint32_t bodySize = packet->m_nBodySize;

  AMFObject obj;
  if (AMF_Decode(&obj, body, bodySize, false) < 0) {
    spdlog::error("[RTMPServer] {}: error decoding metadata packet",
                  clientInfo.address);
    return false;
  }

  AMF_Dump(&obj);
  AVal metastring;
  AMFProp_GetString(AMF_GetProp(&obj, nullptr, 0), &metastring);

  bool success = false;
  do {
    bool setFrameData = false;

    if (AVMATCH(&metastring, &av_setDataFrame)) {
      if (streamInfo.streamID < 0) {
        spdlog::warn("[RTMPServer] {}: received @setDataFrame before "
                     "createStream, ignoring",
                     clientInfo.address);
        break;
      }

      setFrameData = true;
      AMFProp_GetString(AMF_GetProp(&obj, nullptr, 1), &metastring);
    }

    if (AVMATCH(&metastring, &av_onMetaData)) {
      if (setFrameData) {
        AMFObjectProperty prop;
        AVal av;

        bool hasWidth = false, hasHeight = false, hasVideoDataRate = false,
             hasFrameRate = false, hasVideoCodecID = false;

        STR2AVAL(av, "width");
        if (RTMP_FindFirstMatchingProperty(&obj, &av, &prop)) {
          streamInfo.width = static_cast<size_t>(prop.p_vu.p_number);
          hasWidth = true;
        }
        STR2AVAL(av, "height");
        if (RTMP_FindFirstMatchingProperty(&obj, &av, &prop)) {
          streamInfo.height = static_cast<size_t>(prop.p_vu.p_number);
          hasHeight = true;
        }
        STR2AVAL(av, "videodatarate");
        if (RTMP_FindFirstMatchingProperty(&obj, &av, &prop)) {
          streamInfo.videoDataRate = prop.p_vu.p_number;
          hasVideoDataRate = true;
        }
        STR2AVAL(av, "framerate");
        if (RTMP_FindFirstMatchingProperty(&obj, &av, &prop)) {
          streamInfo.frameRate = prop.p_vu.p_number;
          hasFrameRate = true;
        }
        STR2AVAL(av, "videocodecid");
        if (RTMP_FindFirstMatchingProperty(&obj, &av, &prop)) {
          int codecID = static_cast<int>(prop.p_vu.p_number);
          AVCodecID avCodecID = AV_CODEC_ID_NONE;
          const char *bsfName = nullptr;
          if (codecID == RTMP_VIDEOCODEC_H264) { // only H.264 right now
            avCodecID = AV_CODEC_ID_H264;
            // H.264 in FLV is usually in AVCC format, so we need to convert it
            // to Annex B
            bsfName = "h264_mp4toannexb";
          } else {
            spdlog::error(
                "[RTMPServer] {}: unknown/unsupported video codec ID {}",
                clientInfo.address, codecID);
            break;
          }

          streamInfo.codec = avcodec_find_decoder(avCodecID);
          if (!streamInfo.codec) {
            spdlog::error(
                "[RTMPServer] {}: no decoder found for video codec ID {}",
                clientInfo.address, codecID);
            break;
          }
          if (bsfName) {
            streamInfo.bsf = av_bsf_get_by_name(bsfName);
            if (!streamInfo.bsf) {
              spdlog::error("[RTMPServer] {}: bitstream filter {} not found "
                            "for video codec ID {}",
                            clientInfo.address, bsfName, codecID);
              break;
            }
          }

          AVCodecContext *codecContext =
              avcodec_alloc_context3(streamInfo.codec);
          if (!codecContext) {
            spdlog::error("[RTMPServer] {}: failed to allocate codec context "
                          "for video codec ID {}",
                          clientInfo.address, codecID);
            break;
          }

          AVBSFContext *bsfContext = nullptr;
          if (streamInfo.bsf) {
            if (av_bsf_alloc(streamInfo.bsf, &bsfContext) < 0) {
              spdlog::error("[RTMPServer] {}: failed to allocate bitstream "
                            "filter context",
                            clientInfo.address);
              avcodec_free_context(&codecContext);
              break;
            }

            if (avcodec_parameters_from_context(bsfContext->par_in,
                                                codecContext) < 0) {
              spdlog::error("[RTMPServer] {}: failed to copy codec parameters "
                            "to bitstream filter context",
                            clientInfo.address);
              avcodec_free_context(&codecContext);
              av_bsf_free(&bsfContext);
              break;
            }

            if (av_bsf_init(bsfContext) < 0) {
              spdlog::error("[RTMPServer] {}: failed to initialize bitstream "
                            "filter context",
                            clientInfo.address);
              avcodec_free_context(&codecContext);
              av_bsf_free(&bsfContext);
              break;
            }
          }

          {
            std::lock_guard<std::mutex> lk(streamMutex);
            activeStreams[streamInfo.name] =
                std::make_shared<CodecContext>(codecContext, bsfContext);
          }

          hasVideoCodecID = true;
        }

        if (hasWidth && hasHeight && hasVideoDataRate && hasFrameRate &&
            hasVideoCodecID) {
          streamInfo.hasReceivedMetadata = true;
          spdlog::info("[RTMPServer] {}: stream {} metadata: {}x{}, data rate "
                       "{:.2f} kbps, frame rate {:.2f} fps, codec {}",
                       clientInfo.address, streamInfo.streamID,
                       streamInfo.width, streamInfo.height,
                       streamInfo.videoDataRate, streamInfo.frameRate,
                       streamInfo.codec->name);
        } else {
          spdlog::error("[RTMPServer] {}: incomplete metadata for stream {}",
                        clientInfo.address, streamInfo.streamID);
          break;
        }
      }
    }

    success = true;

  } while (false);

  AMF_Reset(&obj);
  return success;
}

bool RTMPServer::handleVideoPacket(RTMP *r, RTMPPacket *packet,
                                   StreamInfo &streamInfo,
                                   const ClientInfo &clientInfo) {
  if (!streamInfo.hasReceivedMetadata) {
    spdlog::warn("[RTMPServer] {}: received video packet before metadata for "
                 "stream {} (\"{}\"), ignoring",
                 clientInfo.address, streamInfo.streamID,
                 streamInfo.name.c_str());
    return true;
  }

  const auto *body = reinterpret_cast<const uint8_t *>(packet->m_body);
  size_t bodySize = packet->m_nBodySize;

  // uint8_t frameType = (body[0] & 0xF0) >> 4;
  uint8_t codecID = body[0] & 0x0F;

  if (codecID != RTMP_VIDEOCODEC_H264) {
    spdlog::error("[RTMPServer] {}: received video packet with unsupported "
                  "codec ID {} for stream {} (\"{}\")",
                  clientInfo.address, codecID, streamInfo.streamID,
                  streamInfo.name.c_str());
    return false;
  }

  uint8_t avcPacketType = body[1];
  // uint32_t compositionTime = (body[2] << 16) | (body[3] << 8) | body[4];

  std::shared_ptr<CodecContext> codecContext;
  {
    std::lock_guard<std::mutex> lk(streamMutex);
    codecContext = activeStreams.at(streamInfo.name);
  }

  // Sequence header
  if (avcPacketType == 0) {
    const uint8_t *avcConfig = body + 5;
    size_t avcConfigSize = bodySize - 5;

    std::lock_guard<std::mutex> lk(codecContext->mutex);
    AVCodecContext *ctx = codecContext->context;

    auto *extraData = reinterpret_cast<uint8_t *>(
        av_malloc(avcConfigSize + AV_INPUT_BUFFER_PADDING_SIZE));
    memcpy(extraData, avcConfig, avcConfigSize);
    memset(extraData + avcConfigSize, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    ctx->extradata = extraData;
    ctx->extradata_size = static_cast<int>(avcConfigSize);

    if (avcodec_is_open(ctx)) {
      spdlog::warn("[RTMPServer] {}: codec context for stream {} (\"{}\") "
                   "already open, ignoring new sequence header",
                   clientInfo.address, streamInfo.streamID, streamInfo.name);
    } else if (avcodec_open2(ctx, streamInfo.codec, nullptr) < 0) {
      spdlog::error("[RTMPServer] {}: failed to open codec context for video "
                    "codec ID {} for stream {} (\"{}\")",
                    clientInfo.address, codecID, streamInfo.streamID,
                    streamInfo.name);
      avcodec_free_context(&codecContext->context);
      return false;
    }

    return true;
  }
  // Video data
  else if (avcPacketType == 1) {

    AVPacket *avPacket = av_packet_alloc();
    avPacket->data = const_cast<uint8_t *>(body + 5);
    avPacket->size = static_cast<int>(bodySize - 5);

    std::lock_guard<std::mutex> lk(codecContext->mutex);

    AVCodecContext *ctx = codecContext->context;
    AVBSFContext *bsfCtx = codecContext->bsfContext;

    if (!avcodec_is_open(codecContext->context)) {
      spdlog::error("[RTMPServer] {}: received video data packet before "
                    "sequence header for stream {} (\"{}\")\n",
                    clientInfo.address, streamInfo.streamID, streamInfo.name);
      return false;
    }

    bool success = false;
    do {
      int ret;
      if (bsfCtx) {
        if ((ret = av_bsf_send_packet(bsfCtx, avPacket)) < 0) {
          spdlog::error("[RTMPServer] {}: failed to send packet to bitstream "
                        "filter for stream {} (\"{}\"): {}\n",
                        clientInfo.address, streamInfo.streamID,
                        streamInfo.name, ret);
          break;
        }

        if ((ret = av_bsf_receive_packet(bsfCtx, avPacket)) < 0) {
          spdlog::error("[RTMPServer] {}: failed to receive packet from "
                        "bitstream filter for stream {} (\"{}\"): {}\n",
                        clientInfo.address, streamInfo.streamID,
                        streamInfo.name, ret);
          break;
        }
      }

      if ((ret = avcodec_send_packet(ctx, avPacket)) < 0) {
        if (ret !=
            AVERROR(EAGAIN)) { // EAGAIN just means it needs more packets.
          spdlog::error("[RTMPServer] {}: failed to send packet to decoder for "
                        "stream {} (\"{}\"): {}",
                        clientInfo.address, streamInfo.streamID,
                        streamInfo.name, ret);
          break;
        }
      }

      success = true;

    } while (false);

    av_packet_free(&avPacket);

    return success;
  }

  spdlog::error("[RTMPServer] {}: received video packet with unknown AVC "
                "packet type {} for stream {} (\"{}\")",
                clientInfo.address, avcPacketType, streamInfo.streamID,
                streamInfo.name);
  return false;
}

bool RTMPServer::sendConnectResult(RTMP *r, double txn) {
  RTMPPacket packet;
  char pbuf[384], *pend = pbuf + sizeof(pbuf);
  AVal av;

  packet.m_nChannel = 0x03;
  packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

  char *enc = packet.m_body;
  enc = AMF_EncodeString(enc, pend, &av_result);
  enc = AMF_EncodeNumber(enc, pend, txn);
  *enc++ = AMF_OBJECT;

  STR2AVAL(av, "FMS/3,5,1,525");
  enc = AMF_EncodeNamedString(enc, pend, &av_fmsVer, &av);
  enc = AMF_EncodeNamedNumber(enc, pend, &av_capabilities, 31.0);
  enc = AMF_EncodeNamedNumber(enc, pend, &av_mode, 1.0);
  *enc++ = 0;
  *enc++ = 0;
  *enc++ = AMF_OBJECT_END;

  *enc++ = AMF_OBJECT;

  STR2AVAL(av, "status");
  enc = AMF_EncodeNamedString(enc, pend, &av_level, &av);
  STR2AVAL(av, "NetConnection.Connect.Success");
  enc = AMF_EncodeNamedString(enc, pend, &av_code, &av);
  STR2AVAL(av, "Connection succeeded.");
  enc = AMF_EncodeNamedString(enc, pend, &av_description, &av);
  enc = AMF_EncodeNamedNumber(enc, pend, &av_objectEncoding, r->m_fEncoding);
#if 0
  STR2AVAL(av, "58656322c972d6cdf2d776167575045f8484ea888e31c086f7b5ffbd0baec55ce442c2fb");
  enc = AMF_EncodeNamedString(enc, pend, &av_secureToken, &av);
#endif
  AMFObjectProperty p, op;
  STR2AVAL(p.p_name, "version");
  STR2AVAL(p.p_vu.p_aval, "3,5,1,525");
  p.p_type = AMF_STRING;
  AMFObject obj;
  obj.o_num = 1;
  obj.o_props = &p;
  op.p_type = AMF_OBJECT;
  STR2AVAL(op.p_name, "data");
  op.p_vu.p_object = obj;
  enc = AMFProp_Encode(&op, enc, pend);
  *enc++ = 0;
  *enc++ = 0;
  *enc++ = AMF_OBJECT_END;

  packet.m_nBodySize = enc - packet.m_body;

  return RTMP_SendPacket(r, &packet, false);
}

bool RTMPServer::sendResultNumber(RTMP *r, double txn, double id) {
  RTMPPacket packet;
  char pbuf[256], *pend = pbuf + sizeof(pbuf);

  packet.m_nChannel = 0x03;
  packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
  packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

  char *enc = packet.m_body;
  enc = AMF_EncodeString(enc, pend, &av_result);
  enc = AMF_EncodeNumber(enc, pend, txn);
  *enc++ = AMF_NULL;
  enc = AMF_EncodeNumber(enc, pend, id);

  packet.m_nBodySize = enc - packet.m_body;

  return RTMP_SendPacket(r, &packet, false);
}

bool RTMPServer::sendPublish(RTMP *r, int streamID) {
  RTMPPacket packet;
  char pbuf[512], *pend = pbuf + sizeof(pbuf);
  AVal av;

  packet.m_nChannel = 0x04;
  packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
  packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = streamID;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

  char *enc = packet.m_body;
  enc = AMF_EncodeString(enc, pend, &av_onStatus);
  enc = AMF_EncodeNumber(enc, pend, 0);
  *enc++ = AMF_NULL;

  *enc++ = AMF_OBJECT;
  STR2AVAL(av, "status");
  enc = AMF_EncodeNamedString(enc, pend, &av_level, &av);
  STR2AVAL(av, "NetStream.Publish.Start");
  enc = AMF_EncodeNamedString(enc, pend, &av_code, &av);
  STR2AVAL(av, "Started publishing.");
  enc = AMF_EncodeNamedString(enc, pend, &av_description, &av);
  *enc++ = 0;
  *enc++ = 0;
  *enc++ = AMF_OBJECT_END;

  packet.m_nBodySize = enc - packet.m_body;
  return RTMP_SendPacket(r, &packet, false);
}

void RTMPServer::avFrameToCvMat(const AVFrame *avFrame, cv::Mat &output) {
  SwsContext *conversion =
      sws_getContext(avFrame->width, avFrame->height,
                     static_cast<AVPixelFormat>(avFrame->format),
                     avFrame->width, avFrame->height, AV_PIX_FMT_BGR24,
                     SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

  output = cv::Mat(avFrame->height, avFrame->width, CV_8UC3);
  uint8_t *dest[4] = {output.data, nullptr, nullptr, nullptr};
  int destStride[4] = {static_cast<int>(output.step[0]), 0, 0, 0};

  sws_scale(conversion, avFrame->data, avFrame->linesize, 0, avFrame->height,
            dest, destStride);

  sws_freeContext(conversion);
}
