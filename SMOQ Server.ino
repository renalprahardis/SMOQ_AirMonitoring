
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266mDNS.h>

ESP8266WiFiMulti WiFiMulti;
MDNSResponder mdns;


WebSocketsServer webSocket = WebSocketsServer(81);
int isi, value, konek = 0;
int asap, karbon, ozon, sulfur, nitro, suhu;

uint8_t cl;


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch (type) {
    case WStype_DISCONNECTED:
      konek = 0;
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        cl = num;
        konek = 1;
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");

      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      konek = 1;
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\n", num, length);
      hexdump(payload, length);
      break;
  }

}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
  WiFiMulti.addAP("SSID", "PASSWORD");
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  WiFi.softAP("Air Monitoring NOW!", "");
  Serial.println(WiFi.hostname("renal"));

  if (mdns.begin("websockethost", WiFi.localIP())) {
    Serial.println("MDNS responder started");
    mdns.addService("http", "tcp", 80);
    mdns.addService("ws", "tcp", 81);
  }
  else {
    Serial.println("MDNS.begin failed");
  }
  Serial.println(WiFi.localIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  data();
}

void data() {
  while (true) {
    webSocket.loop();
    if (Serial.available()) {
      webSocket.loop();
      for (int i = 1; i < 7; i++) {
        isi = Serial.parseFloat();
        //read Data
        if (i == 1) {
          asap = isi;
        } else if (i == 2) {
          karbon = isi;
        } else if (i == 3) {
          ozon = isi;
        } else if (i == 4) {
          sulfur = isi;
        } else if (i == 5) {
          nitro = isi;
        } else if (i == 6) {
          suhu = isi;
        }
        delay(1);
      }
    } else {
      return;
    }

    String payload = "{";
    payload += "\"asap\":"; payload += asap; payload += ",";
    payload += "\"karbon\":"; payload += karbon; payload += "," ;
    payload += "\"ozon\":"; payload += ozon; payload += ",";
    payload += "\"sulfur\":"; payload += sulfur; payload += ",";
    payload += "\"nitrogen\":"; payload += nitro; payload += ",";
    payload += "\"suhu\":"; payload; payload += suhu;
    payload += "}";

    char attributes [100];
    payload.toCharArray( attributes, 100 );
    Serial.println( attributes );
    webSocket.sendTXT(cl, attributes);
    delay(1500);
    break;
  }
}

