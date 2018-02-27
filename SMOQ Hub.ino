#include "DHT.h"
#include "Fsm.h"
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfSpiDriver.h>
#include <MirfHardwareSpiDriver.h>

const char payload_length = 32;
byte data[payload_length];
String minta = "req";
int statuss = 0;
int LedH = 2;
int LedM = 3;
int LedB = 4;

State waiting_data(&get_data, &request, NULL);
Fsm fsm_waiting(&waiting_data);

void request() {
  while (true) {
    //while (Serial.available()) {
    char x = Serial.read();
    if (x == 'x') {
      String sensorVal = String(minta);
      char data[32];
      sensorVal.toCharArray(data, 32);
      Mirf.send((byte*) data);
//      Serial.println("Send request");
      while (Mirf.isSending()) {
        /* code */
      }
      digitalWrite(LedH, LOW);
      digitalWrite(LedB, HIGH);
      delay(1000);
      digitalWrite(LedB, LOW);
    }
    break;
  }
}

void get_data() {
  while (true) {
    
    if (!Mirf.isSending() && Mirf.dataReady()) {
//      Serial.println("Get Data Success");
      Mirf.getData(data);
      Serial.println((char*)data);
      digitalWrite(LedM,HIGH);
      digitalWrite(LedH,LOW);
      //Serial.print(" ");
      delay(1700);
    }else{
      digitalWrite(LedM,LOW);
      digitalWrite(LedH, HIGH);
    }
    request();
  }
}

void sendData(String sensorParam) {
  String sensorVal = String(sensorParam);
  char data[32];
  sensorVal.toCharArray(data,32);
  Mirf.send((byte*) data);
  while (Mirf.isSending()) {
    /* code */
  }
  delay(1000);
}

void setup() {
  Serial.begin(115200); 
//  pinMode(DHTPIN, INPUT);
 pinMode(LedH, OUTPUT);
  pinMode(LedM, OUTPUT);
  pinMode(LedB, OUTPUT);
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
  Mirf.setTADDR((byte *)"serve");
  Mirf.setRADDR((byte *)"serve");
  Mirf.payload = payload_length;
  Mirf.channel = 123;
  Mirf.config();

}

void loop() {
  fsm_waiting.run_machine();
}
