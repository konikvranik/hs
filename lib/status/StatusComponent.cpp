/*
 * StatusComponent.cpp
 *
 *  Created on: 27. 6. 2017
 *      Author: hpa
 */

#include "StatusComponent.h"

StatusComponent::StatusComponent(String nodeid, AbstractComponent *components[],
								 uint8_t ccount, Print *logOutput) : AbstractComponent(nodeid, -1)
{
	this->logOutput = logOutput;
	init(nodeid, components, ccount);
}

void StatusComponent::init(String nodeid, AbstractComponent *components[],
						   uint8_t ccount)
{
	nodeId = nodeid;
	this->components = components;
	this->components_count = ccount;
}

StatusComponent::StatusComponent(String nodeid, AbstractComponent *components[],
								 uint8_t ccount, ESP8266WebServer *webServer) : AbstractComponent(nodeid, -1)
{
	this->webServer = webServer;
	init(nodeid, components, ccount);
}

StatusComponent::~StatusComponent()
{
	// TODO Auto-generated destructor stub
}

void StatusComponent::setup()
{
	if (webServer != NULL)
	{
		webServer->on("/status.json", HTTP_GET, [&]() {
			webServer->send(200, "application/json; charset=utf-8",
							this->ModulesJson());
		});
		webServer->on("/prometheus", HTTP_GET, [&]() {
			webServer->send(200, "text/plain; version=0.0.4; charset=utf-8",
							this->prometheusReport());
		});
		webServer->on("/reset", HTTP_GET, [&]() {
			webServer->send(200, "text/plain", "Resetting...");
			webServer->close();
			yield();
			delay(1000);
			ESP.restart();
		});
		for (int i = 0; i < this->components_count; i++)
		{
			this->components[i]->registerRest(*webServer);
		}
	}
}

String StatusComponent::ModulesStatus()
{
	String result = "";
	for (int i = 0; i < this->components_count; i++)
	{
		StaticJsonBuffer<1000> jsonBuffer;
		JsonObject &jo = jsonBuffer.createObject();
		this->components[i]->reportStatus(jo);
		if (jo.size() > 0)
		{
			result += "\n" + components[i]->moduleName() + ":\n";
			for (auto it = jo.begin(); it != jo.end(); ++it)
			{
				String str;
				it->value.prettyPrintTo(str);
				result += String(it->key) + String(": ") + str + String("\n");
			}

			result += "\n------------------------------------\n";
		}
	}
	return result;
}

String StatusComponent::ModulesJson()
{
	StaticJsonBuffer<2000> jsonBuffer;
	JsonObject &p = jsonBuffer.createObject();
	JsonObject &m = p.createNestedObject("modules");
	for (int i = 0; i < this->components_count; i++)
	{
		JsonObject &jo = m.createNestedObject(components[i]->moduleName());
		this->components[i]->reportStatus(jo);
	}
	jsonPrefix(p);
	jsonSuffix(p);
	String s;
	p.prettyPrintTo(s);
	return s;
}

void StatusComponent::jsonPrefix(JsonObject &p)
{
	p["Node ID"] = nodeId;
	p["Out Queue"] = makeTopic("");
	p["In Queue"] = makeTopic("");
	JsonObject &time = p.createNestedObject("Time");
	time["date"] = NTP.getDateStr();
	time["time"] = NTP.getTimeStr();
	time["DST"] = NTP.isSummerTime() ? "Summer" : "Winter";
	JsonObject &uptime = p.createNestedObject("Uptime");
	uptime["uptime"] = NTP.getUptimeString();
	uptime["since"] = NTP.getTimeDateString(NTP.getFirstSync());
	p["WiFi"] = WiFi.isConnected() ? "connected" : "not connected";
}

void StatusComponent::jsonSuffix(JsonObject &p)
{
	JsonObject &flash = p.createNestedObject("flash");
	flash["ID"] = ESP.getFlashChipId();
	flash["real size"] = ESP.getFlashChipRealSize();
	flash["size by chip ID"] = ESP.getFlashChipSizeByChipId();
	flash["size"] = ESP.getFlashChipSize();
	flash["mode"] = ESP.getFlashChipMode();

	JsonObject &version = p.createNestedObject("version");
	version["Core"] = ESP.getCoreVersion();
	version["Boot"] = ESP.getBootVersion();
	version["SDK"] = ESP.getSdkVersion();

	JsonObject &reset = p.createNestedObject("reset");
	reset["reason"] = ESP.getResetInfoPtr()->reason;
	reset["exccause"] = ESP.getResetInfoPtr()->exccause;
	reset["exccause"] = ESP.getResetInfoPtr()->excvaddr;
	reset["depc"] = ESP.getResetInfoPtr()->depc;
	reset["epc1"] = ESP.getResetInfoPtr()->epc1;
	reset["epc2"] = ESP.getResetInfoPtr()->epc2;
	reset["epc3"] = ESP.getResetInfoPtr()->epc3;

	p["Chip ID"] = ESP.getChipId();
	p["Boot mode"] = ESP.getBootMode();
	p["Cycle count"] = ESP.getCycleCount();
	p["CPU freq"] = ESP.getCpuFreqMHz();
	p["VCC"] = ESP.getVcc() / 1000.000;
	p["Loops"] = lps < 0 ? "UNKNOWN" : String(lps);
	JsonArray &files = p.createNestedArray("FS");
	Dir dir = SPIFFS.openDir("/");
	while (dir.next())
		files.add(dir.fileName() + ":" + dir.fileSize());
#ifdef DEVELOPMENT
	p["development"] = true;
#endif
}

void StatusComponent::loop()
{
	loop_count++;
	uint32_t now = millis();
	if (last_log + LOG_PERIOD < now)
	{
		lps = loop_count * 1000 / (now - last_log);
		loop_count = 0;
		last_log = now;
	}
}

String StatusComponent::moduleName()
{
	return "status";
}

String StatusComponent::prometheusReport()
{
	Prometheus *p = new Prometheus("uptime", NTP.getUptime(), "counter",
								   "Uptime of node " str(NODE_ID));
	p->attribute("unit", "s");
	String result = p->to_string(true);
	delete p;
	p = new Prometheus("loops", lps, "counter", "Number of loops per second");
	p->attribute("unit", "s⁻¹");
	result += p->to_string(true);
	delete p;
	p = new Prometheus("boot_mode", ESP.getBootMode(), "untyped",
					   "Boot mode of ESP8266");
	result += p->to_string(true);
	delete p;
	p = new Prometheus("reset_reason", ESP.getResetInfoPtr()->reason, "untyped",
					   "Reason of reset");
	result += p->to_string(true);
	delete p;
	p = new Prometheus("exception_cause", ESP.getResetInfoPtr()->exccause,
					   "untyped", "Cause of exception");
	result += p->to_string(true);
	delete p;
	p = new Prometheus("exception_address", ESP.getResetInfoPtr()->excvaddr,
					   "untyped", "Address where exception occurs");
	result += p->to_string(true);
	delete p;
	p = new Prometheus("depc", ESP.getResetInfoPtr()->depc, "untyped", "depc");
	result += p->to_string(true);
	delete p;
	p = new Prometheus("epc1", ESP.getResetInfoPtr()->epc1, "untyped", "epc1");
	result += p->to_string(true);
	delete p;
	p = new Prometheus("epc2", ESP.getResetInfoPtr()->epc2, "untyped", "epc2");
	result += p->to_string(true);
	delete p;
	p = new Prometheus("epc3", ESP.getResetInfoPtr()->epc3, "untyped", "epc3");
	result += p->to_string(true);
	delete p;
	p = new Prometheus("cycle_count", ESP.getCycleCount(), "counter",
					   "ESP cycle count");
	result += p->to_string(true);
	delete p;
	p = new Prometheus("cpu_freq", ESP.getCpuFreqMHz(), "gauge",
					   "ESP CPU frequency in MHz");
	result += p->to_string(true);
	delete p;
	p = new Prometheus("esp_power_supply", ESP.getVcc() / 1000.000, "gauge",
					   "ESP voltage in V");
	p->attribute("unit", "V");
	p->attribute("type", "voltage");
	result += p->to_string(true);
	delete p;
	for (int i = 0; i < this->components_count; i++)
	{
		result += this->components[i]->prometheus();
	}
	return result;
}

String StatusComponent::moduleNames()
{
	String result = "";
	for (int i = 0; i < this->components_count; i++)
	{
		result += ((i > 0 ? ", " : "") + this->components[i]->moduleName());
	}
	return result;
}
