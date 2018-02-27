#include "Fsm.h"
#include "DHT.h"
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfSpiDriver.h>
#include <MirfHardwareSpiDriver.h>

#define         MQ2PIN                       (0)
#define         MQ7PIN                       (1)
#define         MQ131PIN                     (2)
#define         MQ136PIN                     (3)
#define         DHTPIN                       (6)

#define         RL_VALUE                     (1)
#define         RL_VALUE_MQ7                 (1)
#define         RL_VALUE_MQ131               (1)
#define         RL_VALUE_MQ136               (1)

//define the load resistance on the board, in kilo ohms
#define         RO_CLEAN_AIR_FACTOR_MQ2      (9.577)
#define         RO_CLEAN_AIR_FACTOR_MQ7      (26.09)
#define         RO_CLEAN_AIR_FACTOR_MQ131    (20)
#define         RO_CLEAN_AIR_FACTOR_MQ136    (20)
//which is derived from the chart in datasheet

#define         CALIBARATION_SAMPLE_TIMES     (40)
#define         CALIBRATION_SAMPLE_INTERVAL  (200)
//cablibration phase
#define         READ_SAMPLE_INTERVAL         (2)
#define         READ_SAMPLE_TIMES            (2)
//normal operation

/**********************Application Related Macros**********************************/
#define         GAS_HYDROGEN                  (0)
#define         GAS_LPG                       (1)
#define         GAS_METHANE                   (2)
#define         GAS_CARBON_MONOXIDE           (3)
#define         GAS_ALCOHOL                   (4)
#define         GAS_SMOKE                     (5)
#define         GAS_PROPANE                   (6)
#define         GAS_O3                        (7)
#define         GAS_SO2                       (8)
#define         GAS_NO2                       (9)
#define         accuracy                      (0)   //for linearcurves

/*****************************Globals************************************************/
DHT dht(DHTPIN, DHT22);
float Ro2 = 0, Ro7 = 10, Ro131 = 0, Ro136 = 0;                           //Ro is initialized to 10 kilo ohms
const char payload_length = 32;
byte data[payload_length];
String dataMsg;
int mq_pin2, a, b, c, d, e, f, kondisi = 0;
float x;

//Events
#define TRIGGER_EVENT 0

State state_calibration(&start_on, NULL, NULL);
State state_reading(&akuisisi_data, NULL, NULL);
Fsm fsm(&state_calibration);

unsigned long time1, time2, waktu;

//start on
void start_on() {
  time1 = millis();
  Serial.print("Calibrating...\n");
  MQResistanceCalculation(f);
  Ro2 = MQCalibration2(MQ2PIN);                       //Calibrating the sensor. Please make sure the sensor is in clean air
  Ro7 = MQCalibration7(MQ7PIN);
  Ro131 = MQCalibration131(MQ131PIN);
  Ro136 = MQCalibration136(MQ136PIN);

  Serial.print("Calibration is done...\n");
  Serial.print("Ro MQ2=");
  Serial.print(Ro2);
  Serial.print("kohm");
  Serial.print("\n");
  Serial.print("Ro MQ7=");
  Serial.print(Ro7);
  Serial.print("kohm");
  Serial.print("\n");
  Serial.print("Ro MQ131=");
  Serial.print(Ro131);
  Serial.print("kohm");
  Serial.print("\n");
  Serial.print("Ro MQ136=");
  Serial.print(Ro136);
  Serial.print("kohm");
  Serial.print("\n");

  Serial.println();
  time2 = millis();
  waktu = time2 - time1;
  Serial.print("Waktu komputasi : ");
  Serial.print(waktu);
  Serial.println(" ms");
  Serial.println("NRF Aktif");
  fsm.trigger(TRIGGER_EVENT);
}

//akuisisi data
void akuisisi_data() {
  while (true) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    a = MQGetGasPercentage2(MQRead(MQ2PIN) / Ro2, GAS_SMOKE);
    b = MQGetGasPercentage7(MQRead(MQ7PIN) / Ro7, GAS_CARBON_MONOXIDE);
    c = MQGetGasPercentage131(MQRead(MQ131PIN) / Ro131, GAS_O3);
    d = MQGetGasPercentage136(MQRead(MQ136PIN) / Ro136, GAS_SO2);
    e = MQGetGasPercentage131(MQRead(MQ131PIN) / Ro131, GAS_NO2);

    dataMsg = String(a) + " " +
              String(b) + " " +
              String(c) + " "  +
              String(d) + " " +
              String(e) + " " +
              String(t);
    Serial.println(dataMsg);
    delay(1500);
    float analog = analogRead(MQ131PIN);
    float voltage = (analog / 1024) * 5;
    send_server(dataMsg);
//    kondisi_buruk();
//    terima();
  }
}

void kondisi_buruk() {
  while (true) {
    if (a > 150 or  b > 10 or c > 253 or  d > 365 or  e > 100) {
      kondisi = 1;
      Serial.println("Kondisi Buruk !!");
      send_server(dataMsg);
    }
    break;
  }
}

void send_server(String sensorParam) {
  Serial.println("Send Data Success");
  while (true) {
    String sensorVal = String(sensorParam);
    char data[32];
    sensorVal.toCharArray(data, 32);
    Mirf.send((byte*) data);
    //    Serial.println("kirim");
    while (Mirf.isSending()) {
      /* code */
    }
    break;
  }
}

void terima() {
  //akuisisi_data();
  while (true) {
    if (!Mirf.isSending() && Mirf.dataReady()) {
      Mirf.getData(data);
      Serial.println("Get Request");
      send_server(dataMsg);
      delay(1700);
    }
    break;
  }
}

void setup() {
  Serial.begin(115200);                               //UART setup, baudrate = 9600bps
  pinMode(DHTPIN, INPUT);
  dht.begin();
  fsm.add_transition(&state_calibration, &state_reading, TRIGGER_EVENT, NULL);
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
  Mirf.setTADDR((byte *)"serve");
  Mirf.payload = payload_length;
  Mirf.channel = 123;
  Mirf.config();
}

void loop() {
  fsm.run_machine();
}

float MQResistanceCalculation(int raw_adc)
{
  return ( ((float)RL_VALUE * (1023 - raw_adc) / raw_adc));
}

float MQCalibration2(int mq_pin2) {
  int i;
  float RS_AIR_val2 = 0, r02;

  for (i = 0; i < CALIBARATION_SAMPLE_TIMES; i++) {               //take multiple samples
    RS_AIR_val2 += MQResistanceCalculation(analogRead(mq_pin2));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  RS_AIR_val2 = RS_AIR_val2 / CALIBARATION_SAMPLE_TIMES;            //calculate the average value

  r02 = RS_AIR_val2 / RO_CLEAN_AIR_FACTOR_MQ2;                    //RS_AIR_val divided by Ro2_CLEAN_AIR_FACTOR yields the Ro2
  //according to the chart in the datasheet
  return r02;
}

float MQCalibration7(int mq_pin7)
{
  int i;
  float RS_AIR_val7 = 0, r07;

  for (i = 0; i < CALIBARATION_SAMPLE_TIMES; i++) {               //take multiple samples
    RS_AIR_val7 += MQResistanceCalculation(analogRead(mq_pin7));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  RS_AIR_val7 = RS_AIR_val7 / CALIBARATION_SAMPLE_TIMES;            //calculate the average value

  r07 = RS_AIR_val7 / RO_CLEAN_AIR_FACTOR_MQ7;                    //RS_AIR_val divided by RO_CLEAN_AIR_FACTOR yields the Ro
  //according to the chart in the datasheet

  return r07;
}

float MQCalibration131(int mq_pin131)
{
  int i;
  float RS_AIR_val131 = 0, r0131;

  for (i = 0; i < CALIBARATION_SAMPLE_TIMES; i++) {               //take multiple samples
    RS_AIR_val131 += MQResistanceCalculation(analogRead(mq_pin131));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  RS_AIR_val131 = RS_AIR_val131 / CALIBARATION_SAMPLE_TIMES;            //calculate the average value

  r0131 = RS_AIR_val131 / RO_CLEAN_AIR_FACTOR_MQ131;                    //RS_AIR_val divided by RO_CLEAN_AIR_FACTOR yields the Ro
  //according to the chart in the datasheet

  return r0131;
}

float MQCalibration136(int mq_pin136)
{
  int i;
  float RS_AIR_val136 = 0, r0136;

  for (i = 0; i < CALIBARATION_SAMPLE_TIMES; i++) {               //take multiple samples
    RS_AIR_val136 += MQResistanceCalculation(analogRead(mq_pin136));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  RS_AIR_val136 = RS_AIR_val136 / CALIBARATION_SAMPLE_TIMES;            //calculate the average value

  r0136 = RS_AIR_val136 / RO_CLEAN_AIR_FACTOR_MQ136;                    //RS_AIR_val divided by RO_CLEAN_AIR_FACTOR yields the Ro
  //according to the chart in the datasheet

  return r0136;
}

float MQRead(int mq_pin)
{
  int i;
  float rs = 0;

  for (i = 0; i < READ_SAMPLE_TIMES; i++) {
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }

  rs = rs / READ_SAMPLE_TIMES;

  return rs;
}

int MQGetGasPercentage2(float rs_ro_ratio2, int gas_id2)
{
  if ( accuracy == 0 ) {
    if ( gas_id2 == GAS_CARBON_MONOXIDE ) {
      return (pow(10, ((-2.955 * (log10(rs_ro_ratio2))) + 4.400)));
    } else if ( gas_id2 == GAS_SMOKE ) {
      return (pow(10, ((-2.331 * (log10(rs_ro_ratio2))) + 3.596)));
    }
  }
  else if ( accuracy == 1 ) {
    if ( gas_id2 == GAS_CARBON_MONOXIDE ) {
      return (pow(10, ((-2.955 * (log10(rs_ro_ratio2))) + 4.457)));
    } else if ( gas_id2 == GAS_SMOKE ) {
      return (pow(10, (-0.976 * pow((log10(rs_ro_ratio2)), 2) - 2.018 * (log10(rs_ro_ratio2)) + 3.617)));
    }
  }
  return 0;
}

int MQGetGasPercentage7(float rs_ro_ratio7, int gas_id7)
{
  if ( accuracy == 0 ) {
    if ( gas_id7 == GAS_CARBON_MONOXIDE ) {
      return (pow(10, ((-1.525 * (log10(rs_ro_ratio7))) + 1.994)));
    }
  }
  else if ( accuracy == 1 ) {
    if ( gas_id7 == GAS_CARBON_MONOXIDE ) {
      return (pow(10, ((-1.525 * (log10(rs_ro_ratio7))) + 1.994)));
    }
  }
  return 0;
}

int MQGetGasPercentage131(float rs_ro_ratio131, int gas_id131)
{
  if ( accuracy == 0 ) {
    if ( gas_id131 == GAS_O3 ) {
      return (pow(10, ((-0.8916 * (log10(rs_ro_ratio131))) + 1.5427)));
    } else if ( gas_id131 == GAS_NO2 ) {
      return (pow(10, ((-3.8398 * (log10(rs_ro_ratio131))) + 4.386)));
    }
  }
  else if ( accuracy == 1 ) {
    if ( gas_id131 == GAS_O3 ) {
      return (pow(10, (0.2889 * pow((log10(rs_ro_ratio131)), 2) - 1.0574 * (log10(rs_ro_ratio131)) + 1.5161)));
    } else if ( gas_id131 == GAS_NO2 ) {
      return (pow(10, (-2.4248 * pow((log10(rs_ro_ratio131)), 2) - 0.0374 * (log10(rs_ro_ratio131)) + 2.9193)));
      return 0;
    }
  }
}

int MQGetGasPercentage136(float rs_ro_ratio136, int gas_id136)
{
  if ( accuracy == 0 ) {
    if ( gas_id136 == GAS_SO2 ) {
      return (pow(10, ((-0.8916 * (log10(rs_ro_ratio136))) + 1.5427)));
    }
    else if ( accuracy == 1 ) {
      if ( gas_id136 == GAS_SO2 ) {
        return (pow(10, (0.2889 * pow((log10(rs_ro_ratio136)), 2) - 1.0574 * (log10(rs_ro_ratio136)) + 1.5161)));
      }
      return 0;
    }
  }
}
