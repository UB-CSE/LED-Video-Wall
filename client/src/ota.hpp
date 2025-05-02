#ifndef INCLUDE_SRC_OTA_HPP_
#define INCLUDE_SRC_OTA_HPP_

#define CURRENT_FW_VERSION "0.1.1"

#define VERSION_URL "http://172.20.10.2:8000/firmware/version.txt"
#define FW_URL "http://172.20.10.2:8000/firmware/client.bin"

#define OTA_MIN_VALID_IMAGE_SIZE (500 * 1024) // our binary is currently ~900kB

#ifdef __cplusplus
extern "C" {
#endif

void start_ota_checker(void);

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_SRC_OTA_HPP_
