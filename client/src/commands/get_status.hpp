#ifndef GET_STATUS_H
#define GET_STATUS_H

#include "protocol.hpp"

int get_status(GetStatusMessage *msg);
void send_status();

#endif
