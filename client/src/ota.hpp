#ifndef INCLUDE_SRC_OTA_HPP_
#define INCLUDE_SRC_OTA_HPP_

// This is the exact string that is compared with build/version.txt when
// determining newest version during the OTA task.
#define CURRENT_FW_VERSION "0.1.4"

#define VERSION_URL "http://192.168.1.123:8000/build/version.txt"
#define FW_URL "http://192.168.1.123:8000/build/client.bin"

#define OTA_MIN_VALID_IMAGE_SIZE (500 * 1024) // our binary is currently ~900kB

#ifdef __cplusplus
extern "C" {
#endif

void start_ota_checker(void);

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_SRC_OTA_HPP_
