#ifndef INIT_INCLUDE_h
#define INIT_INCLUDE_h

#define ARDUINO 100
#define CR "\r\n"

#include "config.h"
#include <Arduino.h>
#include <ArduinoLog.h>
#include <ESP8266WebServer.h>
#include <Esp.h>
#include <HardwareSerial.h>
#include <NtpClientLib.h>
#include <TimeLib.h>
#include <WString.h>
#include <string>

#include "AbstractComponent.h"

#define EXPAND(N) QUOTE(N)
#define QUOTE(NODE_ID) #NODE_ID
// Set this node's subscribe and publish topic prefix

// Enable these if your MQTT broker requires usenrame/password


#include <ESP8266WiFi.h>
#include <ESP8266MQTTClient.h>

#ifdef ENABLE_OTA
#include "ota/OTAComponent.h"
#endif
#ifdef ENABLE_BLYNK
#include "blynk/BlynkComponent.h"
#endif
#ifdef ENABLE_RGB
#include "rgb/RGBComponent.h"
#endif
#ifdef ENABLE_IR
#include "ir/IRComponent.h"
#endif
#ifdef ENABLE_ILIFE
#include "ir/iLifeComponent.h"
#endif
#ifdef ENABLE_DHT
#include "ht_sensor/DHTComponent.h"
#endif
#ifdef ENABLE_HTU
#include "ht_sensor/HTUComponent.h"
#endif
#ifdef ENABLE_DS18
#include "ht_sensor/DS18Component.h"
#endif
#ifdef ENABLE_HCSR04
#include "distance/HcSr04Component.h"
#endif

#endif
