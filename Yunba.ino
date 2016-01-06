// This example uses an Arduino Uno together with
// a WiFi Shield to connect to shiftr.io.
//
// You can check on your device after a successful
// connection here: https://shiftr.io/try.
//
// by Jo婵絾娉� G闁烩晝鐏唚iler
// https://github.com/256dpi/arduino-mqtt

#include "SPI.h"

#include <LWiFi.h>
#include <MQTTClient.h>
#include <LWiFiClient.h>

char *ssid = "yunba.io guest";
char *pass = "123456789";

#define WIFI_AP "yunba.io guest" // provide your WIFI_AP name
#define WIFI_PASSWORD "123456789" //provide your WIFI password
#define WIFI_AUTH LWIFI_WPA

const int subLedPin = 10;
const int pubLedPin = 13;


LWiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;

void setup() {
    pinMode(subLedPin, OUTPUT);
    pinMode(pubLedPin, OUTPUT);
    digitalWrite(subLedPin, LOW);
    digitalWrite(pubLedPin, LOW);

  
//  Serial.begin(9600);
//  WiFi.begin(ssid, pass);
    Serial.begin(9600);
    Serial.println("Serial set up");

     
    Serial.println("Connecting to AP");
    while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)))
    {
        Serial.println(" . ");
        delay(1000);
    }
    Serial.println("Connected to AP");   

  client.begin("10.0.1.52", net);

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
 // Serial.print("checking wifi...");
 // while (WiFi.status() != WL_CONNECTED) {
 //   Serial.print(".");
 //   delay(500);
 // }

//  Serial.print("\nconnecting...");
  while (!client.connect("arduino", "try", "try")) {
    Serial.print(".");
  }

  Serial.println("\nconnected!");

  client.subscribe("baidu");
  // client.unsubscribe("/example");
}

void loop() {
  client.loop();

  if(!client.connected()) {
    connect();
  }

  // publish a message roughly every second.
  if(millis() - lastMillis > 20000) {
    lastMillis = millis();
    client.publish("baidu", "world");
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
