
WIFI_SSID := SuTeren
WIFI_PASSWORD := zirafa123
NODE_ID = esp02
#MODULES := DS18 HCSR04 OTA STATUS
MODULES := HC-SR04 DS18 OTA STATUS
INIT_STUFF := pinMode(2,INPUT);

FLAGS := \
  DIST_CHILD_ID=1 \
  TEMP_CHILD_ID=2 \
  DS18_PIN=0 \
  HCSR04_TRIG_PIN=1 \
  HCSR04_ECHO_PIN=3 \

SV = "2.0"
SN = "multisensor $(NODE_ID)"
CORE_PATH := .
BUILDDIR := build

include ${CORE_PATH}/esp01.mk
include ${CORE_PATH}/core.mk
