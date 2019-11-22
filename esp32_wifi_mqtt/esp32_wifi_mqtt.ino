#include <dummy.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;
const int ledPin = 23;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);


// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);
float temperatureC;
float lastTemperatureC;
float highValue = 27;
float lowValue = 18;
bool relayState = false;

const char* ssid = "MTSgarag"; // Enter your WiFi name
const char* password = "21063598"; // Enter WiFi password
const char* mqtt_server = "95.213.252.43";
const int mqtt_port = 1883;
const char* MQTT_USER = "abacus";
const char* MQTT_PASSWORD = "123qwe";
#define MQTT_SERIAL_PUBLISH_CH "ESP32/serialdata/tx"
#define MQTT_SERIAL_RECEIVER_CH "ESP32/serialdata/rx"

WiFiClient wifiClient;

PubSubClient client(wifiClient);


void setup_wifi() {
	delay(10);
	// We start by connecting to a WiFi network
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	randomSeed(micros());
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
}

void reconnect() {
	// Loop until we're reconnected
	while (!client.connected()) {
		Serial.print("Attempting MQTT connection...");
		// Create a random client ID
		String clientId = "ESP32Client-";
		clientId += String(random(0xffff), HEX);
		// Attempt to connect
		if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
			Serial.println("connected");
			//Once connected, publish an announcement...
			client.publish("/icircuit/presence/ESP32/", "hello world");
			// ... and resubscribe
			client.subscribe(MQTT_SERIAL_RECEIVER_CH);
		}
		else {
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}

void callback(char* topic, byte* payload, unsigned int length) {
	Serial.println("-------new message from broker-----");
	Serial.print("channel:");
	Serial.println(topic);
	Serial.print("data:");
	Serial.write(payload, length);

	if (payload[0] == '1') {
		relayState = true;
		Serial.println("relay ON");
		publishSerialData("Relay ON");
		digitalWrite(ledPin, HIGH);
	}
	else if (payload[0] == '0') {
		Serial.println("relay OFF");
		publishSerialData("Relay OFF");
		digitalWrite(ledPin, LOW);
		relayState = false;
	}
	else if (payload[0] == 'l') {
		lowValue = atof((const char*)payload + 1);
		Serial.println("Low value updated");
		publishSerialData("Low value updated");
		
	}
	else if (payload[0] == 'h') {
		highValue = atof((const char*)payload + 1);
		Serial.println("High value updated");
		publishSerialData("High value updated");
	}
	else {
		Serial.println(payload[0]);
	}
	Serial.println();
}

void setup() {
	Serial.begin(115200);
	sensors.begin();
	Serial.setTimeout(500);// Set time out for 
	setup_wifi();
	client.setServer(mqtt_server, mqtt_port);
	client.setCallback(callback);
	reconnect();
	pinMode(ledPin, OUTPUT);
	digitalWrite(ledPin, HIGH);
}

void publishSerialData(char* serialData) {
	if (!client.connected()) {
		reconnect();
	}
	client.publish(MQTT_SERIAL_PUBLISH_CH, serialData);
}

void publishSerialData1(char* serialData) {
	if (!client.connected()) {
		reconnect();
	}
	client.publish(MQTT_SERIAL_PUBLISH_CH, serialData);
}


void loop() {
	if (WiFi.status() == WL_CONNECTED) {
		client.loop();
	}
	else {
		setup_wifi();
	}
	
	sensors.requestTemperatures();
	temperatureC = sensors.getTempCByIndex(0);
	if (temperatureC != lastTemperatureC) {
		lastTemperatureC = temperatureC;
		if (temperatureC > highValue) {
			digitalWrite(ledPin, LOW);
			Serial.println("Relay OFF");
			publishSerialData("Relay OFF");
		}
		else if (temperatureC < lowValue) {
			digitalWrite(ledPin, HIGH);
			Serial.println("Relay ON");
			publishSerialData("Relay ON");
		}
		char tempCh[7];
		dtostrf(temperatureC, 6, 2, tempCh);
		//itoa(temperatureC, tempCh, 10);
		publishSerialData(tempCh);
		Serial.print("temp = ");
		Serial.println(temperatureC);
	}
	if (Serial.available() > 0) {
		char mun[501];
		memset(mun, 0, 501);
		Serial.readBytesUntil('\n', mun, 500);
		publishSerialData(mun);
	}

}