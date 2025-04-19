#ifndef GET_LOGS_H
#define GET_LOGS_H

#include "protocol.hpp"

int send_logs(int sockfd);
int get_logs(GetLogsMessage *msg, int sockfd);

#endif
