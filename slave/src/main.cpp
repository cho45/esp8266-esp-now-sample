#include <Arduino.h>
#include <ESP8266WiFi.h>
extern "C" {
	#include <espnow.h>
	#include <user_interface.h>
}

#define WIFI_DEFAULT_CHANNEL 1

uint8_t mac[] = {0x5C,0xCF,0x7F,0x8,0x37,0xC7};

void printMacAddress(uint8_t* macaddr) {
	Serial.print("{");
	for (int i = 0; i < 6; i++) {
		Serial.print("0x");
		Serial.print(macaddr[i], HEX);
		if (i < 5) Serial.print(',');
	}
	Serial.println("}");
}

void setup() {
	pinMode(13, OUTPUT);

	Serial.begin(74880);
	Serial.println("Initializing...");

	WiFi.mode(WIFI_AP);
	WiFi.softAP("foobar", "12345678", 1, 0);

	uint8_t macaddr[6];
	wifi_get_macaddr(STATION_IF, macaddr);
	Serial.print("mac address (STATION_IF): ");
	printMacAddress(macaddr);

	wifi_get_macaddr(SOFTAP_IF, macaddr);
	Serial.print("mac address (SOFTAP_IF): ");
	printMacAddress(macaddr);

	if (esp_now_init() == 0) {
		Serial.println("init");
	} else {
		Serial.println("init failed");
		ESP.restart();
		return;
	}

	esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
	esp_now_register_recv_cb([](uint8_t *macaddr, uint8_t *data, uint8_t len) {
		Serial.println("recv_cb");

		Serial.print("mac address: ");
		printMacAddress(macaddr);

		Serial.print("data: ");
		for (int i = 0; i < len; i++) {
			Serial.print(" 0x");
			Serial.print(data[i], HEX);
		}
		Serial.println("");
	});
	esp_now_register_send_cb([](uint8_t* macaddr, uint8_t status) {
		Serial.println("send_cb");

		Serial.print("mac address: ");
		printMacAddress(macaddr);

		Serial.print("status = "); Serial.println(status);
	});

	int res = esp_now_add_peer(mac, (uint8_t)ESP_NOW_ROLE_CONTROLLER,(uint8_t)WIFI_DEFAULT_CHANNEL, NULL, 0);

//	esp_now_unregister_recv_cb();
//	esp_now_deinit();
}

void loop() {
}

