#include "core.h"

ADC_MODE(ADC_VCC);

ESP8266WebServer http_server(80);
MQTTClient mqtt;

AbstractComponent *modules[] =
		{
#ifdef ENABLE_OTA
		new OTAComponent(NODE_ID, &http_server),
#endif
#ifdef ENABLE_RGB
		new RGBComponent(RGB_CHILD_ID, RED_PIN, GREEN_PIN, BLUE_PIN),
#endif
#ifdef ENABLE_IR
		new IRComponent(IR_CHILD_ID, IRRX_PIN, IRTX_PIN),
#endif
#ifdef ENABLE_ILIFE
		new iLifeComponent(ILIFE_CHILD_ID, IRTX_PIN),
#endif
#ifdef ENABLE_DHT
		new DHTComponent(TEMP_CHILD_ID, HUM_CHILD_ID, DHT_PIN),
#endif
#ifdef ENABLE_HTU
		new HTUComponent(TEMP_CHILD_ID, HUM_CHILD_ID, HTU_SCL, HTU_SDA),
#endif
#ifdef ENABLE_DS18
		new DS18Component(NODE_ID, TEMP_CHILD_ID, DS18_PIN),
#endif
#ifdef ENABLE_HC-SR04
		new HcSr04Component(String(NODE_ID), DIST_CHILD_ID, HCSR04_TRIG_PIN, HCSR04_ECHO_PIN),
#endif
	};

int module_count =
		sizeof(modules) > 0 ? sizeof(modules) / sizeof(modules[0]) : 0;

#ifdef ENABLE_STATUS
#include "../STATUS/StatusComponent.h"
StatusComponent status_component(NODE_ID, modules, module_count, &http_server);
#endif

// ============================ PRESENTATION ===================
void presentation() {
	// Present locally attached sensors here
	mqtt.publish("/hello", SN", "SV);
	for (int i = 0; i < module_count; i++) {
		Log.trace(
				("Presenting module " + modules[i]->moduleName() + CR).c_str());
		modules[i]->presentation(&mqtt);
		Log.notice(
				("Module " + modules[i]->moduleName() + " presented" CR).c_str());
	}
}

// ======================= RECEIVE MESSAGE ====================
void receive(String topic, String data, bool cont) {
	for (int i = 0; i < module_count; i++) {
		Log.trace(
				("Receiving for module " + modules[i]->moduleName() + CR).c_str());
		modules[i]->receive(topic, data, cont);
	}
}

void onSTAGotIP(WiFiEventStationModeGotIP ipInfo) {
	Log.notice("Got IP: %s" CR, ipInfo.ip.toString().c_str());
	NTP.begin("pool.ntp.org", 1, true);
	NTP.setInterval(63);
	digitalWrite(2, LOW);
	http_server.begin();
}

void onSTADisconnected(WiFiEventStationModeDisconnected event_info) {
	Log.warning("Disconnected from SSID: %s" CR, event_info.ssid.c_str());
	Log.warning("Reason: %d" CR, event_info.reason);
	http_server.stop();
	digitalWrite(2, HIGH);
}

void setupNTP() {
	Log.notice("Initializing NTP...\r\n");
	static WiFiEventHandler e1, e2;
	NTP.begin("pool.ntp.org", 1, true);
	NTP.setInterval(63);
	NTP.onNTPSyncEvent([](NTPSyncEvent_t ntpEvent) {
		if (ntpEvent) {
			Log.error("Time Sync error: ");
			if (ntpEvent == noResponse)
			Log.error("NTP server not reachable\r\n");
			else if (ntpEvent == invalidAddress)
			Log.error("Invalid NTP server address\r\n");
		} else {
			Log.notice("Got NTP time: %s\r\n",
					NTP.getTimeDateString(NTP.getLastNTPSync()).c_str());
		}
	});
	e1 = WiFi.onStationModeGotIP(onSTAGotIP); // As soon WiFi is connected, start NTP Client
	e2 = WiFi.onStationModeDisconnected(onSTADisconnected);
}

// ======================== SETUP ============================
void setup() {
	//	Serial.end();
	//	Serial1.end();
	pinMode(0, OUTPUT);
	pinMode(2, OUTPUT);
	pinMode(15, OUTPUT);
#ifdef INIT_STUFF
	INIT_STUFF
#endif
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
	}
#ifndef DISABLE_LOGGING
	DEBUG_ESP_PORT.begin(115200);
#endif
	Log.begin(LOG_LEVEL_TRACE, &DEBUG_ESP_PORT);
	Log.notice(
			(CR "starting with " + String(module_count) + " modules..." CR).c_str());

	for (int i = 0; i < module_count; i++) {
		Log.trace("Initializing module ");
		Log.trace((modules[i]->moduleName() + CR).c_str());
		modules[i]->setup();
		Log.notice(
				("Module " + modules[i]->moduleName() + " initalized." CR).c_str());
	}
#ifdef ENABLE_STATUS
	Log.trace("Initializing module status" CR);
	status_component.setup();
	Log.notice("Module status initalized." CR);
#endif

	setupNTP();
	Log.notice("NTP initialized." CR);

	mqtt.onConnect([]() {
		presentation();
	});

	mqtt.onData([](String topic, String data, bool cont) {
		receive(topic, data, cont);
	});

	mqtt.begin("mqtt://mqtt.home:1883", { .lwtTopic = "hello", .lwtMsg =
			"offline", .lwtQos = 0, .lwtRetain = 0 });
	mqtt.subscribe("/qos1", 1);

	Log.notice("Setup done." CR);
}

// ====================== MAIN LOOP ===============

void loop() {
	// Send locally attach sensors data here
	Log.verbose("loop start" CR);
	for (int i = 0; i < module_count; i++) {
		Log.verbose(
				("Processing module " + modules[i]->moduleName() + CR).c_str());
		modules[i]->loop();
		Log.verbose(
				("Module " + modules[i]->moduleName() + " processed." CR).c_str());
		yield();
	}
#ifdef ENABLE_STATUS
	Log.verbose("Processing module status" CR);
	status_component.loop();
	Log.verbose("Module status processed." CR);
#endif

	Log.verbose("loop end" CR);
}
