/* Copyright 2018 <hpa@suteren.net>
 * DS18Component.h
 *
 *  Created on: 20. 6. 2017
 *      Author: hpa
 */

#ifndef SRC_HT_SENSOR_DS18COMPONENT_H_
#define SRC_HT_SENSOR_DS18COMPONENT_H_

#include <DallasTemperature.h>
#include <OneWire.h>
#include "../src/AbstractTemperatureComponent.h"
#include "../src/AbstractTemperatureComponent.h"
#include "../src/Prometheus.h"

#define DS18_DELAY 30000
#define MAX_DEVICES 255

class DS18Component : public AbstractComponent,
                      public AbstractTemperatureComponent {
  uint32_t delayMS = 2000, lastRun = 0, conversionTime;
  int16_t pin;
  DallasTemperature sensor;
  String addr2string(DeviceAddress deviceAddr);
  String scratchpad2string(uint8_t *deviceAddr);
  bool deviceIsReady();
  DeviceAddress devices[MAX_DEVICES];
  float temps[MAX_DEVICES];
  uint8_t ds18Count = 0;
  bool request = false, async = false;
  OneWire *_wire;

public:
  DS18Component(String nodeid, const uint8_t, const int16_t);
  virtual ~DS18Component();
  virtual void setup();
  virtual void loop();
  virtual void presentation(MQTTClient* mqtt);
  virtual float getTemperature();
  virtual void reportStatus(JsonObject &);
  String prometheus();
  virtual String moduleName();
};

#endif // SRC_HT_SENSOR_DS18COMPONENT_H_