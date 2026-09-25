#ifndef SET_LEDS_H
#define SET_LEDS_H

#include "protocol.hpp"

int set_leds(const SetLEDsMessage *msg);
int set_leds_batched(const SetLEDsBatchedMessage *msg);

#endif
