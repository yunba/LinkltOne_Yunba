// This example uses an Arduino Uno together with
// a WiFi Shield to connect to shiftr.io.
//
// You can check on your device after a successful
// connection here: https://shiftr.io/try.
//
// by Jo濠电姷鏁告慨鐢割敊閺嶎厼闂い鏍ㄧ矊缁躲倝鏌熼鍡嫹闁哄鐗犻弻銊╂偆閸屾稑顏� G闂傚倸鍊搁崐鐑芥嚄閸洖纾块柣銏㈩焾閻ょ偓绻濋棃娑卞剱闁稿﹤鐖奸弻娑㈡倷閹碱厼濡穕er
// https://github.com/256dpi/arduino-mqtt

#include "SPI.h"

#include <LWiFi.h>
#include <MQTTClient.h>
#include <LWiFiClient.h>
#include <ArduinoJson.h>
char *ssid = "yunba.io";
char *pass = "Hiyunba2013";

char client_id[56];
char username[56];
char password[56];
char device_id[56];

#define WIFI_AP "yunba.io" // provide your WIFI_AP name
#define WIFI_PASSWORD "Hiyunba2013" //provide your WIFI password
#define WIFI_AUTH LWIFI_WPA

const int subLedPin = 10;
const int pubLedPin = 13;

const char yunba_appkey[] = "563c4afef085fc471efdf803";
const char yunba_topic[] = "linkltone";
const char yunba_devid[] = "linkltone_board2";
char url[56];

LWiFiClient net;
MQTTClient client;
unsigned long lastMillis = 0;

StaticJsonBuffer<200> jsonBuffer;

char broker_addr[56];
uint16_t port;

bool get_ip_pair(const char *url, char *addr, uint16_t *port)
{
  char *p = strstr(url, "tcp://");
  if (p) {
    p += 6;
    char *q = strstr(p, ":");
    if (q) {
      int len = strlen(p) - strlen(q);
      if (len > 0) {
        memcpy(addr, p, len);
        //sprintf(addr, "%.*s", len, p);
        *port = atoi(q + 1);
        return true;
      }
    }
  }
  return false;
}

bool get_host_v2(const char *appkey, char *url)
{
  uint8_t buf[256];
  bool rc = false;
  LWiFiClient net_client;
  while (0 == net_client.connect("tick-t.yunba.io", 9977)) {
    Serial.println("Re-connect to tick");
    delay(1000);
  }
  delay(100);

  String data = "{\"a\":\"" + String(appkey) + "\",\"n\":\"1\",\"v\":\"v1.0\",\"o\":\"1\"}";
  int json_len = data.length();
  int len;

  buf[0] = 1;
  buf[1] = (uint8_t)((json_len >> 8) & 0xff);
  buf[2] = (uint8_t)(json_len & 0xff);
  len = 3 + json_len;
  memcpy(buf + 3, data.c_str(), json_len);
  net_client.flush();
  net_client.write(buf, len);

  while (!net_client.available()) {
    Serial.println(json_len, len);
    Serial.println(len);
    Serial.println("wailt data");
    delay(100);
  }

  memset(buf, 0, 256);
  int v = net_client.read(buf, 256);
  if (v > 0) {
    len = (uint16_t)(((uint8_t)buf[1] << 8) | (uint8_t)buf[2]);
    if (len == strlen((char *)(buf + 3))) {
      Serial.println((char *)(&buf[3]));
      JsonObject& root = jsonBuffer.parseObject((char *)&buf[3]);
      if (!root.success()) {
        Serial.println("parseObject() failed");
        goto exit;
      }
      strcpy(url, root["c"]);
      Serial.println(url);
      rc = true;
    }
  }
exit:
  net_client.stop();
  return rc;
}

bool setup_with_appkey_and_devid(const char* appkey, const char *deviceid/*, REG_info *info*/)
{
  uint8_t buf[256];
  bool rc = false;
  LWiFiClient net_client;

  if (appkey == NULL) return false;

  while (0 == net_client.connect("reg-t.yunba.io", 9944)) {
    Serial.println("Re-connect to tick");
    delay(1000);
  }
  delay(100);

  String data;
  if (deviceid == NULL)
    data = "{\"a\": \"" + String(appkey) + "\", \"p\":4}";
  else
    data = "{\"a\": \"" + String(appkey) + "\", \"p\":4, \"d\": \"" + String(deviceid) + "\"}";
  int json_len = data.length();
  int len;

  buf[0] = 1;
  buf[1] = (uint8_t)((json_len >> 8) & 0xff);
  buf[2] = (uint8_t)(json_len & 0xff);
  len = 3 + json_len;
  memcpy(buf + 3, data.c_str(), json_len);
  net_client.flush();
  net_client.write(buf, len);

  while (!net_client.available()) {
    Serial.println(json_len, len);
    Serial.println(len);
    Serial.println(data);
    Serial.println("wailt data");
    delay(100);
  }

  memset(buf, 0, 256);
  int v = net_client.read(buf, 256);
  if (v > 0) {
    len = (uint16_t)(((uint8_t)buf[1] << 8) | (uint8_t)buf[2]);
    if (len == strlen((char *)(buf + 3))) {
      Serial.println((char *)(&buf[3]));
      JsonObject& root = jsonBuffer.parseObject((char *)&buf[3]);
      if (!root.success()) {
        Serial.println("parseObject() failed");
        net_client.stop();
        return false;
      }
      strcpy(username, root["u"]);
      strcpy(password, root["p"]);
      strcpy(client_id, root["c"]);
      Serial.println(username);
      rc = true;
    }
  }

  net_client.stop();
  return rc;
}

void set_alias(const char *alias)
{
  client.publish(",yali", alias);
}

void publish_to_alias(const char *alias, char *message)
{
  String topic = ",yta/" + String(alias);
  client.publish(topic, message);
}

void setup() {
  pinMode(subLedPin, OUTPUT);
  pinMode(pubLedPin, OUTPUT);
  digitalWrite(subLedPin, LOW);
  digitalWrite(pubLedPin, LOW);

  Serial.begin(9600);
  Serial.println("Serial set up");
  Serial.println("Connecting to AP");
  while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD))) {
    Serial.println(" . ");
    delay(1000);
  }
  Serial.println("Connected to AP");

  //TODO: if we can't get reg info and tick info
  get_host_v2(yunba_appkey, url);
  setup_with_appkey_and_devid(yunba_appkey, yunba_devid/*, &info*/);

  get_ip_pair(url, broker_addr, &port);
  client.begin("192.168.2.136", port, net);

  // client.begin(broker_addr, port, net);

  connect();
}

void flash(int ledPin)
{
  /* Flash LED three times. */
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
}

void connect() {
  //  Serial.print("\nconnecting...");
  while (!client.connect(client_id, username, password)) {
    Serial.print(".");
  }

  Serial.println("\nconnected!");
  client.subscribe(yunba_topic);
  // client.unsubscribe("/example");
  set_alias(yunba_devid);
}

void loop() {
  client.loop();

  if (!client.connected()) {
    connect();
  }

  // publish a message roughly every second.
  if (millis() - lastMillis > 20000) {
    lastMillis = millis();
    client.publish(yunba_topic, "world");
    client.publish2ToAlias("PC", "publish2");
    flash(pubLedPin);
    Serial.print("publish one message\r\n");
  }
  delay(100);
}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
  flash(subLedPin);
  Serial.print("incoming: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();
}

void extMessageReceived(EXTED_CMD cmd, int status, String payload, unsigned int length)
{
  flash(subLedPin);
  Serial.print("incoming ext message: ");
  Serial.print(cmd);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();
}




