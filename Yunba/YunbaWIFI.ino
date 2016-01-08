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
#include <LGPS.h>


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

gpsSentenceInfoStruct g_info;

void getGPSData(gpsSentenceInfoStruct &g_info, char* GPS_formatted)
{
//  LGPS.powerOn();
  boolean GPS_fix = false;

 // while (!GPS_fix)
 // {
    LGPS.getData(&g_info);                                      //get the data from the GPS and store it in 'g_info'
    Serial.println((char*)g_info.GPGGA); 
     Serial.println((char*)g_info.GPGSV); 
    GPS_fix = printGPGGA((char*)g_info.GPGGA,GPS_formatted);    //printGPGGA returns TRUE if the GPGGA string returned confirms a GPS fix.
 // }
 // LGPS.powerOff();
}

boolean printGPGGA(char* str, char* GPS_formatted)
{
  char SMScontent[160];
  char latitude[20];
  char lat_direction[1];
  char longitude[20];
  char lon_direction[1];
  char buf[20];
  char time[30];
  const char* p = str;
  p = nextToken(p, 0); // GGA
  p = nextToken(p, time); // Time
  p = nextToken(p, latitude); // Latitude
  p = nextToken(p, lat_direction); // N or S?
  p = nextToken(p, longitude); // Longitude
  p = nextToken(p, lon_direction); // E or W?
  p = nextToken(p, buf); // fix quality
  if (buf[0] == '1')
  {
    // GPS fix
    p = nextToken(p, buf); // number of satellites
    Serial.print("GPS is fixed:");
    Serial.print(atoi(buf));
    Serial.println(" satellite(s) found!");
    strcpy(SMScontent, "GPS fixed, satellites found: ");
    strcat(SMScontent, buf);
    
    const int coord_size = 8;
    char lat_fixed[coord_size],lon_fixed[coord_size];
    convertCoords(latitude,longitude,lat_fixed, lon_fixed,coord_size);
    
    Serial.print("Latitude:");
    Serial.println(lat_fixed);
    Serial.println(lat_direction);
    
    Serial.print("Longitude:");
    Serial.println(lon_fixed);
    Serial.println(lon_direction);
    
    char gmaps_buffer[50];
    sprintf(gmaps_buffer,"\nhttp://maps.google.com/?q=%s%s,%s%s",lat_fixed,lat_direction,lon_fixed,lon_direction);
    
    strcat(SMScontent,gmaps_buffer);
    
    Serial.print("Time: ");
    Serial.println(time);
    strcat(SMScontent, "\nTime: ");
    strcat(SMScontent,time);
    
    strcpy(GPS_formatted, SMScontent);
    return true;
  }
  else
  {
    Serial.println("GPS is not fixed yet.");
    return false;
  }
}

void convertCoords(const char* latitude, const char* longitude, char* lat_return, char* lon_return, int buff_length)
{
  char lat_deg[3];
  strncpy(lat_deg,latitude,2);      //extract the first 2 chars to get the latitudinal degrees
  lat_deg[2] = 0;                   //null terminate
  
  char lon_deg[4];
  strncpy(lon_deg,longitude,3);      //extract first 3 chars to get the longitudinal degrees
  lon_deg[3] = 0;                    //null terminate
  
  int lat_deg_int = arrayToInt(lat_deg);    //convert to integer from char array
  int lon_deg_int = arrayToInt(lon_deg);
  
  // must now take remainder/60
  //this is to convert from degrees-mins-secs to decimal degrees
  // so the coordinates are "google mappable"
  
  float latitude_float = arrayToFloat(latitude);      //convert the entire degrees-mins-secs coordinates into a float - this is for easier manipulation later
  float longitude_float = arrayToFloat(longitude);
  
  latitude_float = latitude_float - (lat_deg_int*100);      //remove the degrees part of the coordinates - so we are left with only minutes-seconds part of the coordinates
  longitude_float = longitude_float - (lon_deg_int*100);
  
  latitude_float /=60;                                    //convert minutes-seconds to decimal
  longitude_float/=60;
  
  latitude_float += lat_deg_int;                          //add back on the degrees part, so it is decimal degrees
  longitude_float+= lon_deg_int;
  
  snprintf(lat_return,buff_length,"%2.3f",latitude_float);    //format the coordinates nicey - no more than 3 decimal places
  snprintf(lon_return,buff_length,"%3.3f",longitude_float);
}

int arrayToInt(const char* char_array)
{
  int temp;
  sscanf(char_array,"%d",&temp);
  return temp;
}

float arrayToFloat(const char* char_array)
{
  float temp;
  sscanf(char_array, "%f", &temp);
  return temp;
}

const char *nextToken(const char* src, char* buf)
{
  int i = 0;
  while (src[i] != 0 && src[i] != ',')
  i++;
  if (buf)
  {
    strncpy(buf, src, i);
    buf[i] = 0;
  }
  if (src[i])
  i++;
  return src + i;
}



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
  LGPS.powerOn();
  
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
 // client.begin("192.168.2.136", port, net);

   client.begin(broker_addr, port, net);

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
     char GPS_formatted[130]; 
  getGPSData(g_info,GPS_formatted);
  //  client.publish(yunba_topic, "world");
 //   client.publish2ToAlias("PC", "publish2");
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




