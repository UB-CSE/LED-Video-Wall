#ifndef SET_LEDS_H
#define SET_LEDS_H

#include "protocol.hpp"

int set_leds(SetLedsMessage *msg);
int set_leds_batched(SetLedsBatchedMessage *msg);

#endif
