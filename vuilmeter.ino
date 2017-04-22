#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#define SAMPLES 600
#define PEAK 60
#define PEAK_SAMPLES 10

// WiFi information
const char ssid[] = "revspace-pub-2.4ghz";
const char pass[] = "";
WiFiClient client;

// MQTT
const char* mqtt_server = "mosquitto.space.revspace.nl";
PubSubClient mqtt_client(client);


// NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

void connectWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  mqtt_client.setServer(mqtt_server, 1883);

  // NTP
  timeClient.begin();
  timeClient.update();
}

void connectMQTT() {
  while (!mqtt_client.connected()) {
    if (mqtt_client.connect("ESP8266Client")) {
      Serial.println("connected to MQTT");
    } else {
      Serial.println("not conneted to MQTT");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("vuilmeter startup");
  Serial.println();

  // connect to WiFi
  connectWiFi();

  // connect to MQTT
  connectMQTT();
}

int counter = SAMPLES;
int avg = 0;
int max_min = 0;
int sample_counter = PEAK_SAMPLES;
int samples[PEAK_SAMPLES];

void loop() {
  if (!mqtt_client.connected()) {
    connectMQTT();
  }

  int ldr = analogRead(A0);
  Serial.print("ldr: ");
  Serial.println(ldr);
  avg += ldr;

  // Max per minute
  if (avg > max_min) {
    max_min = ldr;
  }

  if (counter-- < 0) {
    int avg_total = avg / SAMPLES;
    counter = SAMPLES;
    mqtt_client.publish("revspace/sensors/netvuil/avg_min", String(avg_total).c_str());
    mqtt_client.publish("revspace/sensors/netvuil/max_min", String(max_min).c_str());
    avg = 0;
    max_min = 0;
  }

  if (sample_counter-- < 0) {
    int winner = 0;
    for (int i = 0; i < PEAK_SAMPLES; i++) {
      if (samples[i] > winner) {
        winner = samples[i];
      }

      if (winner > PEAK) {
        mqtt_client.publish("revspace/sensors/netvuil/peak", String(winner).c_str(), true);
        timeClient.update();
	mqtt_client.publish("revspace/sensors/netvuil/peak_time", String(timeClient.getFormattedTime()).c_str(), true);
      }
      sample_counter = 0;
      winner = 0;
    }

  } else {
    samples[sample_counter] = ldr;
  }
  delay(100);
}
