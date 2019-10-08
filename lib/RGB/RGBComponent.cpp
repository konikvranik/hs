/*
 * RGBComponent.cpp
 *
 *  Created on: 20. 6. 2017
 *      Author: hpa
 */

#include "RGBComponent.h"

// RGBComponent::mode_names = { "normal", "daytime", "candle" };

RGBComponent::RGBComponent(const String node_id, const uint8_t sensor_id, const uint16_t red_pin,
						   const uint16_t green_pin, const uint16_t blue_pin) : AbstractComponent(node_id, sensor_id), rgbBlender(red_pin, green_pin, blue_pin)
{
}

RGBComponent::~RGBComponent()
{
	// TODO Auto-generated destructor stub
}

const String RGBComponent::getModeName()
{
	switch (mode)
	{
	case MODE_CANDLE:
		return "candle";
	case MODE_DAYTIME:
		return "daytime";
	case MODE_DEFAULT:
		return "normal";
	}
	return "UNKNOWN";
}

void RGBComponent::sendMessage()
{
	/*
	 send(light_status_msg.set((int16_t)(light_state == LIGHT_ON ? 1 : 0)));
	 if (light_state == LIGHT_ON) {
	 send(dimmer_msg.set(c2b(rgbBlender.GetColor())));
	 send(rgb_msg.set(c2s(rgbBlender.GetColor()).c_str()));
	 send(mode_msg.set(getModeName()));
	 }
	 Log.notice("message: state %d, dimm %d, color %s, mode: %d, result: %T" CR,
	 light_state, c2b(rgbBlender.GetColor()),
	 c2s(rgbBlender.GetColor()).c_str(), mode, result);
	 Log.notice("    rgb: state %d, dimm %d, color %s, mode: %d, result: %T" CR,
	 light_state, c2b(rgbBlender.GetColor()),
	 c2s(rgbBlender.GetColor()).c_str(), mode, result);
	 */
}

void RGBComponent::loop()
{
	if (result)
	{
		switch (mode)
		{
		case MODE_CANDLE:
			rgbBlender.Blend(rgbBlender.GetColor(), candle.color_list[candle.RandColor()], candle.RandTimer());
			result = false;
			break;
		case MODE_DAYTIME:
			const Color new_color = daytimeColor();
			if (new_color != rgbBlender.GetColor())
			{
				rgbBlender.Blend(rgbBlender.GetColor(), new_color, BLEND_TIME);
				result = false;
			}
			break;
		}
	}
	if (!result)
	{
		result = rgbBlender.Update();
		if (result)
		{
			sendMessage();
			switch (mode)
			{
			case MODE_CANDLE:
				candle.Reset();
				break;
			case MODE_DEFAULT:
				if (light_state == LIGHT_ON)
					rgbBlender.Hold(rgb);
				else
					rgbBlender.TurnOff();
				break;
			}
		}
	}
	unsigned long now = millis();
	if (last_light_msg + MSG_PERIOD < now)
	{
		sendMessage();
		last_light_msg = now;
	}
}

void RGBComponent::doOnRest()
{
	bool fail = false;
	if (this->webServer == nullptr)
	{
		return;
	}
	if (this->webServer->hasArg("state"))
	{
		String state = this->webServer->arg("state");
		if (state == "on")
		{
			this->light_state = LIGHT_ON;
		}
		else if (state == "off")
		{
			this->light_state = LIGHT_OFF;
		}
		else
		{
			fail = true;
		}
	}
	if (!fail && this->webServer->hasArg("mode"))
	{
		String req_mode = this->webServer->arg("mode");
		if (req_mode == "candle")
		{
			this->mode = MODE_CANDLE;
			result = true;
		}
		else if (req_mode == "daytime")
		{
			this->mode = MODE_DAYTIME;
			result = true;
		}
		else if (req_mode == "normal")
		{
			this->mode = MODE_DEFAULT;
		}
		else
		{
			fail = true;
		}
	}

	if (fail)
	{
		this->webServer->send(400, "application/json; charset=utf-8", "{ \"error\":\"wrong parameter\" }");
	}
	else
	{
		if (this->webServer->hasArg("color"))
		{
			this->rgb = h2c(this->webServer->arg("color"));
		}
		if (this->webServer->hasArg("red"))
		{
			this->rgb.red = this->webServer->arg("red").toInt();
		}
		if (this->webServer->hasArg("green"))
		{
			this->rgb.green = this->webServer->arg("green").toInt();
		}
		if (this->webServer->hasArg("blue"))
		{
			this->rgb.blue = this->webServer->arg("blue").toInt();
		}
		if (this->webServer->hasArg("intensity"))
		{
			this->rgb = b2c(this->rgb, this->webServer->arg("intensity").toInt());
		}
		if (this->mode == MODE_DEFAULT)
		{
			blend(this->rgb);
		}
		this->webServer->send(200, "application/json; charset=utf-8", String("{ \"r\":") + String(this->rgb.red) + String(", \"g\":") + String(this->rgb.green) + String(",\"b\":") + String(this->rgb.blue) + String(", \"mode\":\"") + String(this->mode) + String("\", \"state\":") + String(this->light_state));
	}
}

void RGBComponent::registerRest(ESP8266WebServer &webServer)
{
	webServer.on(String("/rgb/") + String(this->sensor_id), HTTP_GET, std::bind(&RGBComponent::doOnRest, this));
	this->webServer = &webServer;
}

/*
void RGBComponent::receive(String topic, String data, bool cont)
{

	uint16_t int_val = message.getInt();
	const char *str_val = message.getString();
	char *cust_val = (char *)message.getCustom();
	switch (message.type)
	{
	case V_STATUS:
		Log.notice("V_STATUS command received: %d (%s)" CR, int_val, c2s(rgb).c_str());
		if ((int_val < 0) || (int_val > 1))
		{
			Log.warning("V_STATUS data invalid (should be 0/1)" CR);
			return;
		}
		if (light_state != int_val && int_val == LIGHT_ON)
		{
			mode = MODE_DAYTIME;
		}
		light_state = int_val;
		if (mode == MODE_DEFAULT || light_state == 0)
		{
			blend(rgb);
		}
		else
		{
			result = true;
		}
		break;
	case V_PERCENTAGE:
		Log.notice("V_PERCENTAGE command received: %d (%s)" CR, int_val,
				   c2s(rgb).c_str());
		if (mode == MODE_UNDEF)
			break;
		int_val = constrain(int_val, 0, 100);
		mode = MODE_DEFAULT;
		rgb = b2c(rgb, int_val);
		blend(rgb);
		break;
	case V_RGB:
		Log.notice("V_RGB command received: %s" CR, str_val);
		if (mode == MODE_UNDEF)
			break;
		mode = MODE_DEFAULT;
		rgb = h2c(str_val);
		blend(rgb);
		break;
	case V_TEXT:
		Log.notice("V_TEXT command received: %s" CR, cust_val);
		if (mode == MODE_UNDEF)
			break;
		if (strcmp(cust_val, "candle") == 0)
		{
			mode = MODE_CANDLE;
			result = true;
		}
		else if (strcmp(cust_val, "daytime") == 0)
		{
			mode = MODE_DAYTIME;
			result = true;
		}
		else if (strcmp(cust_val, "normal") == 0)
		{
			mode = MODE_DEFAULT;
			blend(rgb);
		}
		break;
	}
}
*/

String RGBComponent::c2s(const Color color)
{
	String result = String(color.red, HEX) + ":";
	result.concat(String(color.green, HEX) + ":");
	result.concat(String(color.blue, HEX));
	return result;
}

const Color RGBComponent::h2c(const String rgb)
{
	long int color = strtol(rgb.c_str(), NULL, 16);
	int16_t blue = int16_t(color % 0x100);
	color = color / 0x100;
	int16_t green = int16_t(color % 0x100);
	int16_t red = int16_t(color / 0x100);
	return Color(red, green, blue);
}

int16_t colorRange(int16_t color)
{
	return color < 0 ? 0 : ((color > RGB_MAX_VALUE - 1) ? RGB_MAX_VALUE - 1 : color);
}

const Color RGBComponent::cn(Color rgb)
{
	return Color(colorRange(rgb.red), colorRange(rgb.green), colorRange(rgb.blue));
}

const int16_t RGBComponent::c2b(const Color c)
{
	return (c.red + c.green + c.blue) * BRIGHTNESS_MAX_VALUE / 3 / RGB_MAX_VALUE;
}

const Color RGBComponent::b2c(Color rgb, const int16_t brightness)
{
	int16_t a = rgb.red + rgb.green + rgb.blue;
	int16_t d = ((brightness * 3 * RGB_MAX_VALUE / BRIGHTNESS_MAX_VALUE) - a);
	if (d == 0)
		return rgb;
	int16_t c = d > 0 ? RGB_MAX_VALUE : 0;
	return rgb + ((c - rgb) * d / ((3 * c) - a));
}

const Color RGBComponent::daytimeColor()
{
	uint32_t h = hour() * 60 + minute();
	double s = (h - 60) * M_PI / 12 / 60 + M_PI * 1.5;
	int16_t b = 82 + fmax(0, fmin(1, sin(s) * 2 + .5)) * 173;
	b = b * BRIGHTNESS_MAX_VALUE / RGB_MAX_VALUE;
	return b2c(Color(DAYTIMECOLOR), b);
}

void RGBComponent::blend(Color c)
{
	// If last dimmer state is zero, set dimmer to 100
	if (light_state == LIGHT_ON)
	{
	}
	else
	{
		mode = MODE_DEFAULT;
		c = _BLACK;
	}
	rgbBlender.Blend(rgbBlender.GetColor(), c, (uint32_t)BLEND_TIME);
	result = false;
}

void RGBComponent::setup()
{
	rgbBlender.Hold(_BLACK);
}

void RGBComponent::reportStatus(JsonObject &jo)
{
	jo["ID"] = this->sensor_id;
	jo["Mode"] = this->getModeName();
	jo["State"] = this->light_state ? "ON" : "OFF";
	jo["Result"] = this->result ? "TRUE" : "FALSE";
	jo["Dimm"] = String(this->c2b(this->rgb)) + "%";
	JsonObject &c = jo.createNestedObject("Candle");
	c["color"] = this->candle.rand_color;
	c["colorString"] = this->c2s(
		this->candle.color_list[this->candle.rand_color]);
	c["delay"] = this->candle.timer;
	jo["Color"] = "0x" + this->c2s(this->rgb);
	jo["Real color"] = "0x" + this->c2s(this->getColor());
}

const Color RGBComponent::getColor()
{
	return rgbBlender.GetColor();
}

String RGBComponent::moduleName()
{
	return "RGB";
}
