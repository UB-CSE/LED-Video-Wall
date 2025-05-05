#include "esp_log.h"
#include "freertos/ringbuf.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_BUFFER_SIZE 4096

static RingbufHandle_t buffer_handle;
static vprintf_like_t original_vprintf;

int get_buffered_logs(char **logs, size_t *len) {
  if (!logs || !len) {
    return -1;
  }

  size_t free_size = xRingbufferGetCurFreeSize(buffer_handle);
  size_t used_size = LOG_BUFFER_SIZE - free_size;
  *len = used_size;

  // If it's empty, just return a null terminator.
  if (used_size == 0) {
    *logs = (char *)malloc(1);

    if (!*logs) {
      return -1;
    }

    (*logs)[0] = '\0';
    return 0;
  }

  char *buffer = (char *)malloc(used_size + 1);
  if (!buffer) {
    return -1;
  }

  size_t offset = 0;
  size_t current = 0;

  void *bytes = xRingbufferReceiveUpTo(buffer_handle, &current, 0, used_size);
  if (bytes) {
    memcpy(buffer + offset, bytes, current);
    offset += current;
    vRingbufferReturnItem(buffer_handle, bytes);
  }

  // If the data wraps around the ring buffer, we must call
  // "xRingbufferReceiveUpTo" again.
  if (offset < used_size) {
    bytes =
        xRingbufferReceiveUpTo(buffer_handle, &current, 0, used_size - offset);
    if (bytes) {
      memcpy(buffer + offset, bytes, current);
      offset += current;
      vRingbufferReturnItem(buffer_handle, bytes);
    }
  }

  buffer[offset] = '\0';
  *logs = buffer;

  return 0;
}

int buffer_vprintf(const char *fmt, va_list args) {
  va_list tmp;
  va_copy(tmp, args);
  int len = vsnprintf(NULL, 0, fmt, tmp);
  va_end(tmp);

  if (len < 0) {
    return original_vprintf(fmt, args);
  }

  char *buffer = (char *)malloc(len + 1);
  if (!buffer) {
    return original_vprintf(fmt, args);
  }

  va_copy(tmp, args);
  vsnprintf(buffer, len + 1, fmt, tmp);
  va_end(tmp);

  if (len > 0) {
    xRingbufferSend(buffer_handle, buffer, len, 0);
  }

  free(buffer);

  return original_vprintf(fmt, args);
}

void init_buffered_log(void) {
  buffer_handle = xRingbufferCreate(LOG_BUFFER_SIZE, RINGBUF_TYPE_BYTEBUF);
  configASSERT(buffer_handle);

  original_vprintf = esp_log_set_vprintf(buffer_vprintf);
}
