#include "rtmp.hpp"

#include <librtmp/log.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <string_view>

#define NUM_WORKER_THREADS 5

#define SAVC(x) static const AVal av_##x = AVC(#x)

SAVC(app);
SAVC(connect);
SAVC(flashVer);
SAVC(swfUrl);
SAVC(pageUrl);
SAVC(tcUrl);
SAVC(capabilities);
SAVC(audioCodecs);
SAVC(videoCodecs);
SAVC(objectEncoding);
SAVC(_result);
SAVC(createStream);
SAVC(getStreamLength);
SAVC(fmsVer);
SAVC(mode);
SAVC(level);
SAVC(code);
SAVC(description);
// SAVC(secureToken);
SAVC(onStatus);
SAVC(publish);

static const AVal av_NetStream_Authenticate_UsherToken =
    AVC("NetStream.Authenticate.UsherToken");

#define STR2AVAL(av, str)                                                      \
  do {                                                                         \
    av.av_val = str;                                                           \
    av.av_len = strlen(av.av_val);                                             \
  } while (0)

RTMPServer::RTMPServer(int port /*= 1935*/, const char *address /*= "0.0.0.0"*/,
                       const char *cert /*= nullptr*/,
                       const char *key /*= nullptr*/)
    : port(port), address(address) {
  initRTMPLogLevel();

  if (cert && key) {
    sslContext = RTMP_TLS_AllocServerContext(cert, key);
  }

  wasInitSuccessful = startServer();

  if (!wasInitSuccessful) {
    fprintf(stderr, "RTMPServer: failed to start server\n");
  }
}

RTMPServer::~RTMPServer() {
  if (wasInitSuccessful) {
    stopServer();
  }

  if (sslContext) {
    RTMP_TLS_FreeServerContext(sslContext);
  }
}

void RTMPServer::initRTMPLogLevel() {
  const char *logLevelEnv = getenv("RTMP_DEBUG_LEVEL");
  if (logLevelEnv) {
    const std::string_view log_level(logLevelEnv);
    using namespace std::string_view_literals;
    if (log_level == "CRIT"sv) {
      RTMP_debuglevel = RTMP_LOGCRIT;
    } else if (log_level == "ERROR"sv) {
      RTMP_debuglevel = RTMP_LOGERROR;
    } else if (log_level == "WARNING"sv) {
      RTMP_debuglevel = RTMP_LOGWARNING;
    } else if (log_level == "INFO"sv) {
      RTMP_debuglevel = RTMP_LOGINFO;
    } else if (log_level == "DEBUG"sv) {
      RTMP_debuglevel = RTMP_LOGDEBUG;
    } else if (log_level == "DEBUG2"sv) {
      RTMP_debuglevel = RTMP_LOGDEBUG2;
    } else if (log_level == "ALL"sv) {
      RTMP_debuglevel = RTMP_LOGALL;
    }
  }
}

bool RTMPServer::startServer() {
  socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socketFd < 0) {
    perror("RTMPServer: failed to create socket");
    return false;
  }

  do {
    int tmp = 1;
    setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<char *>(&tmp), sizeof(tmp));

    int flags = fcntl(socketFd, F_GETFL, 0);
    if (flags < 0) {
      perror("RTMPServer: failed to get socket flags");
      break;
    }
    if (fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) < 0) {
      perror("RTMPServer: failed to set socket to non-blocking");
      break;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(address);
    addr.sin_port = htons(port);
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

    if (bind(socketFd, reinterpret_cast<struct sockaddr *>(&addr),
             sizeof(struct sockaddr_in)) < 0) {
      perror("RTMPServer: failed to bind socket");
      break;
    }

    if (listen(socketFd, 10) < 0) {
      perror("RTMPServer: failed to listen on socket");
      break;
    }

    serverThread = std::thread(std::bind(&RTMPServer::acceptConnections, this));

    for (size_t i = 0; i < NUM_WORKER_THREADS; i++) {
      workerThreads.emplace_back(
          std::bind(&RTMPServer::workerThreadFunc, this));
    }

    return true;

  } while (false);

  // Failure
  close(socketFd);
  return false;
}

void RTMPServer::stopServer() {
  {
    std::lock_guard<std::mutex> lk(queueMutex);
    clientQueue = std::queue<int>();
    isActive = false;
  }
  queueCondition.notify_all();

  for (std::thread &worker : workerThreads) {
    worker.join();
  }
  serverThread.join();

  if (close(socketFd) < 0) {
    perror("RTMPServer: failed to close socket");
  }
}

void RTMPServer::acceptConnections() {
  isActive = true;

  while (isActive) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int clientSocketFd =
        accept(socketFd, reinterpret_cast<struct sockaddr *>(&addr), &addrlen);

    if (clientSocketFd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No incoming connection.
      } else {
        perror("RTMPServer: failed to accept connection");
      }
      continue;
    }

    printf("RTMPServer: accepted connection from %s\n",
           inet_ntoa(addr.sin_addr));

    handleNewConnection(clientSocketFd);
  }
}

void RTMPServer::handleNewConnection(int clientSocketFd) {
  {
    std::lock_guard<std::mutex> lk(queueMutex);
    clientQueue.push({clientSocketFd});
  }
  queueCondition.notify_one();
}

void RTMPServer::workerThreadFunc() {
  while (isActive) {
    int clientSocketFd;

    {
      std::unique_lock<std::mutex> lk(queueMutex);
      queueCondition.wait(lk,
                          [this] { return !isActive || !clientQueue.empty(); });
      if (!isActive) {
        break;
      }

      clientSocketFd = clientQueue.front();
      clientQueue.pop();
    }

    handleClient(clientSocketFd);
  }
}

void RTMPServer::handleClient(int clientSocketFd) {
  // Timeout for http requests
  struct timeval tv;
  memset(&tv, 0, sizeof(struct timeval));
  tv.tv_sec = 5;

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(clientSocketFd, &fds);

  int streamID = -1;

  do {
    if (select(clientSocketFd + 1, &fds, nullptr, nullptr, &tv) <= 0) {
      perror("RTMPServer: timeout waiting for client request");
      break;
    }

    RTMP *rtmp = RTMP_Alloc();
    do {
      RTMP_Init(rtmp);
      rtmp->m_sb.sb_socket = clientSocketFd;
      if (sslContext && !RTMP_TLS_Accept(rtmp, sslContext)) {
        fprintf(stderr, "RTMPServer: TLS handshake failed\n");
        break;
      }
      if (!RTMP_Serve(rtmp)) {
        fprintf(stderr, "RTMPServer: handshake failed\n");
        break;
      }
      RTMPPacket packet = {0};
      while (RTMP_IsConnected(rtmp) && RTMP_ReadPacket(rtmp, &packet)) {
        if (!RTMPPacket_IsReady(&packet)) {
          continue;
        }
        bool result = handlePacket(rtmp, &packet, &streamID);

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
}

bool RTMPServer::handlePacket(RTMP *r, RTMPPacket *packet, int *streamID) {
  RTMP_Log(RTMP_LOGDEBUG, "received packet type %02X, size %u bytes",
           packet->m_packetType, packet->m_nBodySize);

  switch (packet->m_packetType) {
  case RTMP_PACKET_TYPE_CHUNK_SIZE:
    if (packet->m_nBodySize >= 4) {
      r->m_inChunkSize = AMF_DecodeInt32(packet->m_body);
    }
    break;
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
    printf("video packet, size %u bytes, not fully supported\n",
           packet->m_nBodySize);
    // TODO
    break;
  case RTMP_PACKET_TYPE_FLEX_STREAM_SEND:
    break;
  case RTMP_PACKET_TYPE_FLEX_SHARED_OBJECT:
    break;
  case RTMP_PACKET_TYPE_FLEX_MESSAGE:
    RTMP_Log(RTMP_LOGDEBUG, "flex message, size %u bytes, not fully supported",
             packet->m_nBodySize);
    return handleInvoke(r, packet, 1, streamID);
  case RTMP_PACKET_TYPE_INFO:
    break;
  case RTMP_PACKET_TYPE_SHARED_OBJECT:
    break;
  case RTMP_PACKET_TYPE_INVOKE:
    RTMP_Log(RTMP_LOGDEBUG, "received: invoke %u bytes", packet->m_nBodySize);
    return handleInvoke(r, packet, 0, streamID);
  case RTMP_PACKET_TYPE_FLASH_VIDEO:
    break;
  default:
    RTMP_Log(RTMP_LOGDEBUG, "unknown packet type received: 0x%02x",
             packet->m_packetType);
    break;
  }

  return true;
}

bool RTMPServer::handleInvoke(RTMP *r, RTMPPacket *packet, unsigned int offset,
                              int *streamID) {
  const char *body = packet->m_body + offset;
  unsigned int bodySize = packet->m_nBodySize - offset;

  if (body[0] != 0x02) {
    RTMP_Log(RTMP_LOGWARNING,
             "sanity failed. no string method in invoke packet");
    return false;
  }

  AMFObject obj;
  if (AMF_Decode(&obj, body, bodySize, false) < 0) {
    fprintf(stderr, "RTMPServer: error decoding invoke packet\n");
    return false;
  }

  AMF_Dump(&obj);
  AVal method;
  AMFProp_GetString(AMF_GetProp(&obj, nullptr, 0), &method);
  double txn = AMFProp_GetNumber(AMF_GetProp(&obj, nullptr, 1));
  RTMP_Log(RTMP_LOGDEBUG, "client invoking <%s>", method.av_val);

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
        if (!r->Link.app.av_val)
          r->Link.app.av_val = "";
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
    {
      std::lock_guard<std::mutex> lk(streamMutex);
      *streamID = ++lastStreamID;
    }
    sendResultNumber(r, txn, *streamID);
  } else if (AVMATCH(&method, &av_getStreamLength)) {
    sendResultNumber(r, txn, 10.0);
  } else if (AVMATCH(&method, &av_NetStream_Authenticate_UsherToken)) {
    AVal av_dquote, av_escdquote;
    STR2AVAL(av_dquote, "\"");
    STR2AVAL(av_escdquote, "\\\"");

    AVal usherToken;
    AMFProp_GetString(AMF_GetProp(&obj, nullptr, 3), &usherToken);
    avReplace(&usherToken, &av_dquote, &av_escdquote);
    r->Link.usherToken = usherToken;
  } else if (AVMATCH(&method, &av_publish)) {
    if (*streamID != -1) {
      sendPublish(r, *streamID);
    }
  }
  AMF_Reset(&obj);
  return true;
}

bool RTMPServer::sendConnectResult(RTMP *r, double txn) {
  RTMPPacket packet;
  char pbuf[384], *pend = pbuf + sizeof(pbuf);
  AVal av;

  packet.m_nChannel = 0x03; // control channel (invoke)
  packet.m_headerType = 1;  /* RTMP_PACKET_SIZE_MEDIUM; */
  packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

  char *enc = packet.m_body;
  enc = AMF_EncodeString(enc, pend, &av__result);
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

  packet.m_nChannel = 0x03; // control channel (invoke)
  packet.m_headerType = 1;  /* RTMP_PACKET_SIZE_MEDIUM; */
  packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = 0;
  packet.m_hasAbsTimestamp = 0;
  packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

  char *enc = packet.m_body;
  enc = AMF_EncodeString(enc, pend, &av__result);
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
  packet.m_headerType = 0;
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

void RTMPServer::avReplace(AVal *src, const AVal *orig, const AVal *repl) {
  char *srcbeg = src->av_val;
  char *srcend = src->av_val + src->av_len;
  int n = 0;

  char *sptr = src->av_val;
  while (sptr < srcend && (sptr = strstr(sptr, orig->av_val))) {
    n++;
    sptr += orig->av_len;
  }
  if (!n)
    return;

  char *dest = new char[src->av_len + 1 + (repl->av_len - orig->av_len) * n];

  sptr = src->av_val;
  char *dptr = dest;
  while (sptr < srcend && (sptr = strstr(sptr, orig->av_val))) {
    n = sptr - srcbeg;
    memcpy(dptr, srcbeg, n);
    dptr += n;
    memcpy(dptr, repl->av_val, repl->av_len);
    dptr += repl->av_len;
    sptr += orig->av_len;
    srcbeg = sptr;
  }
  n = srcend - srcbeg;
  memcpy(dptr, srcbeg, n);
  dptr += n;
  *dptr = '\0';
  src->av_val = dest;
  src->av_len = dptr - dest;
}
