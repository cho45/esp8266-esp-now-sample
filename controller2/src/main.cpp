#include <Arduino.h>
#include <ESP8266WiFi.h>
extern "C" {
	#include <espnow.h>
	#include <user_interface.h>
}

#include "rtc_memory.hpp"

#define WIFI_DEFAULT_CHANNEL 1

uint8_t mac[] = {0x1A,0xFE,0x34,0xEE,0x84,0x88};

struct deep_sleep_data_t {
	uint16_t count = 0;
	uint8_t  send = 0;
	uint16_t data[12];

	void add_data(uint16_t n) {
		data[count] = n;
	}

	template <class T>
	void run_every_count(uint16_t n, T func) {
		count++;
		if (!send) {
			send = count % (n - 1) == 0;
		} else {
			send = 0;
			count = 0;
			func();
		}
	}
};
rtc_memory<deep_sleep_data_t> deep_sleep_data;

void printMacAddress(uint8_t* macaddr) {
	Serial.print("{");
	for (int i = 0; i < 6; i++) {
		Serial.print("0x");
		Serial.print(macaddr[i], HEX);
		if (i < 5) Serial.print(',');
	}
	Serial.println("}");
}

void post_sensor_data();

void setup() {
	pinMode(13, OUTPUT);

	Serial.begin(74880);
	Serial.println("Initializing...");

	// データ読みこみ
	if (!deep_sleep_data.read()) {
		Serial.println("system_rtc_mem_read failed");
	}
	Serial.print("deep_sleep_data->count = ");
	Serial.println(deep_sleep_data->count);

	// データの変更処理(任意)
	deep_sleep_data->add_data(deep_sleep_data->count);

	deep_sleep_data->run_every_count(4, [&]{
		Serial.println("send data");
		// なんか定期的に書きこみたい処理
		post_sensor_data();
	});

	if (!deep_sleep_data.write()) {
		Serial.print("system_rtc_mem_write failed");
	}

	if (deep_sleep_data->send) {
		ESP.deepSleep(2.5e6, WAKE_RF_DEFAULT);
	} else {
		// sendしない場合は WIFI をオフで起動させる
		ESP.deepSleep(2.5e6, WAKE_RF_DISABLED);
	}

//	esp_now_unregister_recv_cb();
//	esp_now_deinit();
}

void loop() {
}

void post_sensor_data() {
	WiFi.mode(WIFI_STA);

	uint8_t macaddr[6];
	wifi_get_macaddr(STATION_IF, macaddr);
	Serial.print("mac address (STATION_IF): ");
	printMacAddress(macaddr);

	wifi_get_macaddr(SOFTAP_IF, macaddr);
	Serial.print("mac address (SOFTAP_IF): ");
	printMacAddress(macaddr);

	if (esp_now_init()==0) {
		Serial.println("direct link  init ok");
	} else {
		Serial.println("dl init failed");
		ESP.restart();
		return;
	}

	esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
	esp_now_register_recv_cb([](uint8_t *macaddr, uint8_t *data, uint8_t len) {
		Serial.println("recv_cb");

		Serial.print("mac address: ");
		printMacAddress(macaddr);

		Serial.print("data: ");
		for (int i = 0; i < len; i++) {
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

	int res = esp_now_add_peer(mac, (uint8_t)ESP_NOW_ROLE_SLAVE,(uint8_t)WIFI_DEFAULT_CHANNEL, NULL, 0);

	esp_now_send(mac, (uint8_t*)deep_sleep_data->data, sizeof(deep_sleep_data->data));
}
