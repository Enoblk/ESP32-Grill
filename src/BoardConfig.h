#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Original ESP32 controller wiring. This remains the default pinout.
#if defined(GRILL_BOARD_ESP32S3_WROOM)

#define GRILL_BOARD_NAME       "ESP32-S3 WROOM"

#ifndef RELAY_IGNITER_PIN
#define RELAY_IGNITER_PIN     4
#endif
#ifndef RELAY_AUGER_PIN
#define RELAY_AUGER_PIN       5
#endif
#ifndef RELAY_HOPPER_FAN_PIN
#define RELAY_HOPPER_FAN_PIN  6
#endif
#ifndef RELAY_BLOWER_FAN_PIN
#define RELAY_BLOWER_FAN_PIN  7
#endif
#ifndef MAX31865_CS_PIN
#define MAX31865_CS_PIN       10
#endif
#ifndef SDA_PIN
#define SDA_PIN               8
#endif
#ifndef SCL_PIN
#define SCL_PIN               9
#endif
#ifndef AMBIENT_TEMP_PIN
#define AMBIENT_TEMP_PIN      1
#endif

#else

#define GRILL_BOARD_NAME       "ESP32"

#ifndef RELAY_IGNITER_PIN
#define RELAY_IGNITER_PIN     27
#endif
#ifndef RELAY_AUGER_PIN
#define RELAY_AUGER_PIN       26
#endif
#ifndef RELAY_HOPPER_FAN_PIN
#define RELAY_HOPPER_FAN_PIN  25
#endif
#ifndef RELAY_BLOWER_FAN_PIN
#define RELAY_BLOWER_FAN_PIN  14
#endif
#ifndef MAX31865_CS_PIN
#define MAX31865_CS_PIN       5
#endif
#ifndef SDA_PIN
#define SDA_PIN               21
#endif
#ifndef SCL_PIN
#define SCL_PIN               22
#endif
#ifndef AMBIENT_TEMP_PIN
#define AMBIENT_TEMP_PIN      36
#endif

#endif

#endif
