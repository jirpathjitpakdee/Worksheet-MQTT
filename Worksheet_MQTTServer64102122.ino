#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <uri/UriBraces.h>
#include <uri/UriRegex.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <PubSubClient.h>

const char *ssid = "iPhone";
const char *password = "4321zszs";
const char *mqttServer = "172.20.10.3";



ESP8266WebServer server(4000);
DHT dht14(D4, DHT11);
const int led = D6;
bool led_status = false;
WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");


void init_wifi(String ssid, String password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


void reconnect() {
  while (!client.connected()) {
    Serial.print(("Attemping MQTT connection..."));
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("LED");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }


  if (message.equals("on")) {
    digitalWrite(led, HIGH);
  } else if (message.equals("off")) {
    digitalWrite(led, LOW);
  }
}


void setup(void) {
  pinMode(led, OUTPUT);
  dht14.begin();
  Serial.begin(115200);
  client.setServer(mqttServer, 1883);
  init_wifi(ssid, password);
  timeClient.begin();
  timeClient.setTimeOffset(7 * 3600);
  pinMode(led, OUTPUT);
  client.setCallback(callback);

  server.on("/", HTTP_GET, []() {
    String html =
      "<html><head>"
      "<script>"
      "  setInterval(function() {"
      "    fetch('/GetData')"
      "      .then(response => response.json())"
      "      .then(data => {"
      "        fetch('http://172.20.10.3:3000/data', {"
      "          method: 'POST',"
      "          headers: {"
      "            'Content-Type': 'application/json'"
      "          },"
      "          body: JSON.stringify(data)"
      "        });"
      "      });"
      "  }, 10000);"
      "</script>"
      "</head><body>"
      "</body></html>";

    server.send(200, "text/html", html);
  });

  server.on("/GetData", HTTP_GET, []() {
    float humid = dht14.readHumidity();
    float temp = dht14.readTemperature();

    timeClient.update();
    String dateTime = getFormattedDateTime();

    String json = "{\"datetime\":\"" + dateTime + "\",\"humid\":" + String(humid) + ",\"temp\":" + String(temp) + "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  static unsigned long lastMillis = 0;
  if (millis() - lastMillis > 10000) {
    float humid = dht14.readHumidity();
    float temp = dht14.readTemperature();

   timeClient.update();
    String dateTime = getFormattedDateTime();

    String data = "{\"datetime\":\"" + dateTime + "\",\"humid\":" + String(humid) + ",\"temp\":" + String(temp) + "}";

    client.publish("dht11", data.c_str());

    lastMillis = millis();
  }

  server.handleClient();
}

String getFormattedDateTime() {
  timeClient.update();
  time_t now = timeClient.getEpochTime();
  struct tm *timeinfo = localtime(&now);

  char formattedDateTime[25];
  strftime(formattedDateTime, sizeof(formattedDateTime), "%Y-%m-%d %H:%M:%S", timeinfo);

  return String(formattedDateTime);
}