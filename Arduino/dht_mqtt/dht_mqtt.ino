/****************************************
 * Include Libraries
 ****************************************/

#include <PubSubClient.h>
#include <DHT.h>
#include <ESP8266WiFi.h>

/****************************************
 * Define Constants
 ****************************************/

#define BOARD_NAME "esp8266"

#define DHT11_NAME "DHT11"
#define DHT22_NAME "DHT22"

#define USE_DHT11
//#define USE_DHT22

#define MQTT_DHT11_SUBTOPIC_NAME "dht11"
#define MQTT_DHT22_SUBTOPIC_NAME "dht22"
#define MQTT_PUBLISH_INTERVAL_MS 500

#define DHT_ERROR_NONE 0

/****************************************
 * Function prototypes
 ****************************************/

void setupWiFi(void);
void setupMQTT(void);
void reconnectMQTT(void);

/****************************************
 * Typedefs
 ****************************************/

typedef struct {
  float h;
  float t;
} dht_measurement_t;

/****************************************
 * Enums
 ****************************************/

enum App_States {
  kIDLE = 0,
  kPUBLISH_MQTT,
};

/****************************************
 * Global variables
 ****************************************/

const char*     g_wifi_ssid = "REPLACE_WITH_YOUR_SSID";
const char*     g_wifi_password = "REPLACE_WITH_YOUR_PASSWORD";

const char      g_mqtt_client_id[] = "";
const char      g_mqtt_broker[] = "";
const char      g_mqtt_username[] = "";
const char      g_mqtt_password[] = "";
char            g_mqtt_topic[50];
char            g_mqtt_payload[150];
unsigned long   g_mqtt_publish_millis = 0;
bool            g_mqtt_published_flag = 0;

WiFiClient      g_wifi_client;
PubSubClient    g_mqtt_client(g_wifi_client);

#ifdef USE_DHT11
DHT             dht11(DHT11, D5);
#endif

#ifdef USE_DHT22
DHT             dht22(DHT22, D6);
#endif

enum App_States g_app_state = kIDLE;

/****************************************
 * Main Functions
 ****************************************/

void setup() {
  Serial.begin(115200);
  Serial.println("DHTxx MQTT test!");

  setupWiFi();
  setupMQTT();

#ifdef USE_DHT11
  dht11.begin();
#endif

#ifdef USE_DHT22
  dht22.begin();
#endif
}

void loop() {
  switch (g_app_state) {
    case kIDLE:
      if (!g_mqtt_client.connected()) {
        reconnectMQTT();
      }

      if ((millis() - g_mqtt_publish_millis) >= MQTT_PUBLISH_INTERVAL_MS) {
        g_app_state = kPUBLISH_MQTT;
      }
      break;

    case kPUBLISH_MQTT:
#ifdef USE_DHT11
      dht_measurement_t dht11_measurement = {
        .h = dht11.readHumidity(),
        .t = dht11.readTemperature()
      };

      Serial.printf("Sensor %s reading:\n", DHT11_NAME);
      if (evaluateDHTReading(dht11_measurement.h, dht11_measurement.t) != DHT_ERROR_NONE) {
        break;
      }

      sprintf(g_mqtt_topic, "%s", "");
      sprintf(g_mqtt_topic, "%s/%s/", BOARD_NAME, MQTT_DHT11_SUBTOPIC_NAME);
      Serial.print("topic: ");
      Serial.println(g_mqtt_topic);

      publishDHTReading(dht11_measurement.t, dht11_measurement.h);
#endif

#ifdef USE_DHT22
      dht_measurement_t dht22_measurement = {
        .h = dht22.readHumidity(),
        .t = dht22.readTemperature()
      };

      Serial.printf("Sensor %s reading:\n", DHT22_NAME);
      if (evaluateDHTReading(dht22_measurement.h, dht22_measurement.t) != DHT_ERROR_NONE) {
        break;
      }

      sprintf(g_mqtt_topic, "%s", "");
      sprintf(g_mqtt_topic, "%s/%s/", BOARD_NAME, MQTT_DHT22_SUBTOPIC_NAME);
      Serial.print("topic: ");
      Serial.println(g_mqtt_topic);

      publishDHTReading(dht22_measurement.t, dht22_measurement.h);
#endif
      g_app_state = kIDLE;
      g_mqtt_publish_millis = millis();
      break;
  }
  g_mqtt_client.loop();
}

void publishDHTReading(float t, float h) {
  sprintf(g_mqtt_payload, "%s", "");
  sprintf(g_mqtt_payload, "{\"temp\": \"%.2f\", \"hum\": %.2f}", t, h);
  Serial.print("payload: ");
  Serial.println(g_mqtt_payload);

  g_mqtt_client.publish(g_mqtt_topic, g_mqtt_payload);
}

uint8_t evaluateDHTReading(float h, float t) {
  if (isnan(h) || isnan(t)) {
    Serial.printf("Failed to read from sensor!");
    return -1;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println("%");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println("°C");

  return DHT_ERROR_NONE;
}

void setupWiFi() {
  Serial.print("Connecting WiFi: ");
  Serial.println(g_wifi_ssid);

  WiFi.begin(g_wifi_ssid, g_wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupMQTT(void) {
  g_mqtt_client.setServer(g_mqtt_broker, 1883);
  g_mqtt_publish_millis = millis();
}

void reconnectMQTT(void) {
  while (!g_mqtt_client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (g_mqtt_client.connect(g_mqtt_client_id, g_mqtt_username, g_mqtt_password)) {
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(g_mqtt_client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}
