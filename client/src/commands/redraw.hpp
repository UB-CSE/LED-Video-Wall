#ifndef REDRAW_H
#define REDRAW_H

#include "freertos/idf_additions.h"
#include "protocol.hpp"

extern TaskHandle_t notify_handle;

void init_redraw();
int redraw(RedrawMessage *msg);

#endif
