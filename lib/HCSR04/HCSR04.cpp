/*
 * HcSr04Component.cpp
 *
 *  Created on: 20. 6. 2017
 *      Author: hpa
 */

#include "HCSR04.h"

HcSr04Component::HcSr04Component(String node_id, const uint8_t sensor_id,
		const int16_t trigPin, const int16_t echoPin) :
		AbstractComponent(node_id, sensor_id) {
	this->trigPin = trigPin, this->echoPin = echoPin;
}

void HcSr04Component::setup() {
	pinMode(trigPin, OUTPUT);
	pinMode(echoPin, INPUT);
	this->sensor = new Ultrasonic(trigPin, echoPin);
}

void HcSr04Component::loop() {
	if (lastRun == 0 || lastRun + delayMS < millis()) {
		double old_distance = this->distance;
		float tmp = this->sensor->distanceRead(CM);
		lastRun = millis();
		if (tmp > 0) {
			this->distance = tmp;
			if (old_distance != this->distance) {
				const String topic = this->makeTopic("");
				const String value = String(this->getDistance());
				Log.trace("topic: %s, value: %s", topic.c_str(), value.c_str());
			}
		}
	}
}

float HcSr04Component::getDistance() {
	return this->distance;
}

void HcSr04Component::reportStatus(JsonObject &jo) {
	jo["ID"] = this->sensor_id;
	jo["topic"] = this->makeTopic("");
	jo["type"] = this->AbstractDistanceComponent::getType();
	jo["value"] = this->getDistance();
	jo["unit"] = "cm";
}

String HcSr04Component::prometheus() {
	if (this->getDistance() <= 0)
		return "";
	Prometheus p("distance", this->getDistance(), "gauge",
			"Distance measured by HC-SR04 ultrasonic sensor.");
	p.attribute("unit", "cm");
	p.attribute("type", "distance");
	return p.to_string(true);
}

String HcSr04Component::moduleName() {
	return "HC-SR04";
}