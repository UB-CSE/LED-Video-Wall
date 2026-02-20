#include "rtmp.hpp"

#include <librtmp/log.h>

#include <arpa/inet.h>
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
    ssl_ctx = RTMP_TLS_AllocServerContext(cert, key);
  }

  init_successful = startServer();

  if (!init_successful) {
    fprintf(stderr, "RTMPServer: failed to start server\n");
  }
}

RTMPServer::~RTMPServer() {
  if (init_successful) {
    stopServer();
  }

  if (ssl_ctx) {
    RTMP_TLS_FreeServerContext(ssl_ctx);
  }
}

void RTMPServer::initRTMPLogLevel() {
  const char *log_level_env = getenv("RTMP_DEBUG_LEVEL");
  if (log_level_env) {
    const std::string_view log_level(log_level_env);
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
  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0) {
    perror("RTMPServer: failed to create socket");
    return false;
  }

  int tmp = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&tmp),
             sizeof(tmp));

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(address);
  addr.sin_port = htons(port);

  if (bind(sockfd, reinterpret_cast<struct sockaddr *>(&addr),
           sizeof(struct sockaddr_in)) < 0) {
    perror("RTMPServer: failed to bind socket");
    return false;
  }

  if (listen(sockfd, 10) < 0) {
    perror("RTMPServer: failed to listen on socket");
    close(sockfd);
    return false;
  }

  server_thread = std::thread(std::bind(&RTMPServer::serverThread, this));
  return true;
}

void RTMPServer::serverThread() {
  state = State::ACCEPTING;

  while (state == State::ACCEPTING) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int client_sockfd =
        accept(sockfd, reinterpret_cast<struct sockaddr *>(&addr), &addrlen);

    if (client_sockfd > 0) {
      printf("RTMPServer: accepted connection from %s\n",
             inet_ntoa(addr.sin_addr));

      serve(client_sockfd);

    } else {
      perror("RTMPServer: failed to accept connection");
    }
  }
  state = State::STOPPED;
}

void RTMPServer::serve(int client_sockfd) {
  state = State::RUNNING;

  RTMP *rtmp = RTMP_Alloc();
  RTMPPacket packet = {0};

  // Timeout for http requests
  struct timeval tv;
  memset(&tv, 0, sizeof(struct timeval));
  tv.tv_sec = 5;

  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(client_sockfd, &fds);

  if (select(client_sockfd + 1, &fds, NULL, NULL, &tv) <= 0) {
    perror("RTMPServer: timeout waiting for client request");
    goto quit;
  } else {
    RTMP_Init(rtmp);
    rtmp->m_sb.sb_socket = client_sockfd;
    if (ssl_ctx && !RTMP_TLS_Accept(rtmp, ssl_ctx)) {
      fprintf(stderr, "RTMPServer: TLS handshake failed\n");
      goto cleanup;
    }
    if (!RTMP_Serve(rtmp)) {
      fprintf(stderr, "RTMPServer: handshake failed\n");
      goto cleanup;
    }
  }
  while (RTMP_IsConnected(rtmp) && RTMP_ReadPacket(rtmp, &packet)) {
    if (!RTMPPacket_IsReady(&packet)) {
      continue;
    }
    servePacket(rtmp, &packet);
    RTMPPacket_Free(&packet);
  }

cleanup:
  RTMP_Close(rtmp);
  /* Should probably be done by RTMP_Close() ... */
  rtmp->Link.playpath.av_val = NULL;
  rtmp->Link.tcUrl.av_val = NULL;
  rtmp->Link.swfUrl.av_val = NULL;
  rtmp->Link.pageUrl.av_val = NULL;
  rtmp->Link.app.av_val = NULL;
  rtmp->Link.flashVer.av_val = NULL;
  if (rtmp->Link.extras.o_num > 0) {
    delete[] rtmp->Link.extras.o_props;
    rtmp->Link.extras.o_num = 0;
  }
  if (rtmp->Link.usherToken.av_val) {
    delete rtmp->Link.usherToken.av_val;
    rtmp->Link.usherToken.av_val = NULL;
  }
  RTMP_Free(rtmp);

quit:
  if (state == State::RUNNING)
    state = State::ACCEPTING;
}

bool RTMPServer::servePacket(RTMP *r, RTMPPacket *packet) {
  bool result = true;

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

    if (!serveInvoke(r, packet, 1)) {
      RTMP_Close(r);
    }
    break;
  case RTMP_PACKET_TYPE_INFO:
    break;
  case RTMP_PACKET_TYPE_SHARED_OBJECT:
    break;
  case RTMP_PACKET_TYPE_INVOKE:
    RTMP_Log(RTMP_LOGDEBUG, "received: invoke %u bytes", packet->m_nBodySize);

    if (!serveInvoke(r, packet, 0)) {
      RTMP_Close(r);
    }
    break;
  case RTMP_PACKET_TYPE_FLASH_VIDEO:
    break;
  default:
    RTMP_Log(RTMP_LOGDEBUG, "unknown packet type received: 0x%02x",
             packet->m_packetType);
    break;
  }

  return result;
}

bool RTMPServer::serveInvoke(RTMP *r, RTMPPacket *packet, unsigned int offset) {
  const char *body = packet->m_body + offset;
  unsigned int body_size = packet->m_nBodySize - offset;

  if (body[0] != 0x02) {
    RTMP_Log(RTMP_LOGWARNING,
             "sanity failed. no string method in invoke packet");
    return false;
  }

  AMFObject obj;
  if (AMF_Decode(&obj, body, body_size, false) < 0) {
    fprintf(stderr, "RTMPServer: error decoding invoke packet\n");
    return false;
  }

  AMF_Dump(&obj);
  AVal method;
  AMFProp_GetString(AMF_GetProp(&obj, NULL, 0), &method);
  double txn = AMFProp_GetNumber(AMF_GetProp(&obj, NULL, 1));
  RTMP_Log(RTMP_LOGDEBUG, "client invoking <%s>", method.av_val);

  if (AVMATCH(&method, &av_connect)) {
    AMFObject cobj;
    AVal pname, pval;

    AMFProp_GetObject(AMF_GetProp(&obj, NULL, 2), &cobj);
    for (int i = 0; i < cobj.o_num; i++) {
      pname = cobj.o_props[i].p_name;
      pval.av_val = NULL;
      pval.av_len = 0;
      if (cobj.o_props[i].p_type == AMF_STRING)
        pval = cobj.o_props[i].p_vu.p_aval;
      if (AVMATCH(&pname, &av_app)) {
        r->Link.app = pval;
        pval.av_val = NULL;
        if (!r->Link.app.av_val)
          r->Link.app.av_val = "";
      } else if (AVMATCH(&pname, &av_flashVer)) {
        r->Link.flashVer = pval;
        pval.av_val = NULL;
      } else if (AVMATCH(&pname, &av_swfUrl)) {
        r->Link.swfUrl = pval;
        pval.av_val = NULL;
      } else if (AVMATCH(&pname, &av_tcUrl)) {
        r->Link.tcUrl = pval;
        pval.av_val = NULL;
      } else if (AVMATCH(&pname, &av_pageUrl)) {
        r->Link.pageUrl = pval;
        pval.av_val = NULL;
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
    sendResultNumber(r, txn, ++stream_id);
  } else if (AVMATCH(&method, &av_getStreamLength)) {
    sendResultNumber(r, txn, 10.0);
  } else if (AVMATCH(&method, &av_NetStream_Authenticate_UsherToken)) {

    AVal av_dquote, av_escdquote;
    STR2AVAL(av_dquote, "\"");
    STR2AVAL(av_escdquote, "\\\"");

    AVal usher_token;
    AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &usher_token);
    avReplace(&usher_token, &av_dquote, &av_escdquote);
    r->Link.usherToken = usher_token;
  } else if (AVMATCH(&method, &av_publish)) {
    sendPublish(r);
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

bool RTMPServer::sendPublish(RTMP *r) {
  RTMPPacket packet;
  char pbuf[512], *pend = pbuf + sizeof(pbuf);
  AVal av;

  packet.m_nChannel = 0x04;
  packet.m_headerType = 0;
  packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
  packet.m_nTimeStamp = 0;
  packet.m_nInfoField2 = stream_id;
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

void RTMPServer::stopServer() {
  if (state != State::STOPPED) {
    if (state == State::RUNNING) {
      state = State::STOPPING;

      // wait for streaming threads to exit
      while (state != State::STOPPED)
        usleep(100000);

      server_thread.join();
    }

    if (close(sockfd) < 0) {
      perror("RTMPServer: failed to close socket");
    }

    state = State::STOPPED;
  }
}

void RTMPServer::avReplace(AVal *src, const AVal *orig, const AVal *repl) {
  char *srcbeg = src->av_val;
  char *srcend = src->av_val + src->av_len;
  int n = 0;

  /* count occurrences of orig in src */
  char* sptr = src->av_val;
  while (sptr < srcend && (sptr = strstr(sptr, orig->av_val))) {
    n++;
    sptr += orig->av_len;
  }
  if (!n)
    return;

  char* dest = new char[src->av_len + 1 + (repl->av_len - orig->av_len) * n];

  sptr = src->av_val;
  char* dptr = dest;
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
