#define BLYNK_TEMPLATE_ID "TMPL6PExIjUrE"
#define BLYNK_TEMPLATE_NAME "Monitoring"
#define BLYNK_AUTH_TOKEN "V2qYflcZNqb4WX_E-7ncNjuzqUb_Yu-Q"

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ModbusMaster.h>

//konfigurasi
const uint8_t DE_PIN      = 18;
const uint8_t RE_PIN      = 19;
const int     RX_PIN      = 16;   //RX2
const int     TX_PIN      = 17;   //TX2
const uint8_t POMPA_PIN   = 5;
const uint8_t SLAVE_ADDR  = 0x01;

//wifi
char ssid[] = "F25";
char pass[] = "bayu9285";

//timing sama ambang
const unsigned long SENSOR_PERIOD_MS = 10UL * 60UL * 1000UL; //pembacaan tiap 10menit
const float         MOISTURE_START   = 50.0;  // pompa on < 50%
const float         MOISTURE_STOP    = 60.0;  // pompa off > 60% 
const unsigned long PUMP_ON_MS       = 10UL * 60UL * 1000UL; // pompa on 10 menit

// komunikasi
ModbusMaster node;
BlynkTimer   timer;
HardwareSerial &RS485 = Serial2;

uint16_t nilai;
float ph = 0, kel = 0, t = 0;

bool          pumpOn     = false;
unsigned long pumpOffAt  = 0;

//rs485
void preTransmission() {
  digitalWrite(DE_PIN, HIGH);
  digitalWrite(RE_PIN, HIGH);
}
void postTransmission() {
  digitalWrite(DE_PIN, LOW);
  digitalWrite(RE_PIN, LOW);
}

//fuzzy logic
float fuzzyPump(float kelembapan) {
  float dry = 0, moist = 0, veryWet = 0;

  // Dry
  if (kelembapan < 20) dry = 1;
  else if (kelembapan <= 50) dry = (50 - kelembapan) / 30.0;

  // Moist
  if (kelembapan >= 50 && kelembapan <= 60)      moist = (kelembapan - 50) / 10.0;
  else if (kelembapan > 60 && kelembapan <= 70)  moist = (70 - kelembapan) / 10.0;

  // Very Wet
  if (kelembapan >= 70 && kelembapan <= 85)      veryWet = (kelembapan - 70) / 15.0;
  else if (kelembapan > 85)                       veryWet = 1;

  float z_fast = 100, z_medium = 60, z_slow = 20;
  float numerator = dry*z_fast + moist*z_medium + veryWet*z_slow;
  float denominator = dry + moist + veryWet;
  if (denominator == 0) return 0;
  return numerator / denominator;
}

//pembacaan modbus
int baca_sensor(uint16_t alamat) {
  int result = node.readHoldingRegisters(alamat, 1);
  if (result == node.ku8MBSuccess) {
    nilai = node.getResponseBuffer(0);
  } else {
    Serial.print("Modbus gagal baca alamat "); Serial.println(alamat);
    nilai = 0;
  }
  return nilai;
}

//one shoot pompa
void pumpUpdate() {
  if (pumpOn && millis() >= pumpOffAt) {
    pumpOn = false;
    digitalWrite(POMPA_PIN, LOW);
    Serial.println("Pompa: OFF (timer habis)");
    Blynk.logEvent("pump_off", "Pompa OFF (timer selesai)");
  }
}

//1 sesi pompa on = 10 menit
void startPumpSession() {
  pumpOn = true;
  pumpOffAt = millis() + PUMP_ON_MS;
  digitalWrite(POMPA_PIN, HIGH); // relay aktif HIGH
  Serial.println("Pompa: ON (mulai sesi 10 menit)");
  Blynk.logEvent("pump_on", "Pompa ON (mulai 10 menit)");
}

//loop data sama ngirim
void doCycle() {
  // Baca sensor
  ph  = baca_sensor(3) / 10.0;
  kel = baca_sensor(0) / 10.0;
  t   = baca_sensor(1) / 10.0;

  // Serial debug
  Serial.println("pH Tanah          : " + String(ph));
  Serial.println("Kelembapan Tanah  : " + String(kel) + " %");
  Serial.println("Suhu Tanah        : " + String(t) + " °C");
  Serial.println("============================================");

  // Kirim ke Blynk
  Blynk.virtualWrite(V0, kel);
  Blynk.virtualWrite(V1, t);
  Blynk.virtualWrite(V2, ph);

  // Fuzzy output (indikator)
  float intensitas = fuzzyPump(kel);
  Blynk.virtualWrite(V3, intensitas);

  // Logika sesi pompa:
  if (kel < MOISTURE_START && !pumpOn) {
    startPumpSession();
  } else if (kel > MOISTURE_STOP && pumpOn) {
    pumpOn = false;
    digitalWrite(POMPA_PIN, LOW);
    Serial.println("Pompa: OFF (kelembapan sudah cukup)");
    Blynk.logEvent("pump_off", "Pompa OFF (kelembapan cukup)");
  }
}

//looping
void setup() {
  // Pin mode
  pinMode(DE_PIN, OUTPUT);
  pinMode(RE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);
  digitalWrite(RE_PIN, LOW);

  pinMode(POMPA_PIN, OUTPUT);
  digitalWrite(POMPA_PIN, LOW); // relay mati (aktif HIGH)

  // Serial monitor
  Serial.begin(115200);
  delay(200);

  // RS485 via HardwareSerial (ESP32)
  RS485.begin(4800, SERIAL_8N1, RX_PIN, TX_PIN);

  // Modbus
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  node.begin(SLAVE_ADDR, RS485);

  // Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // run start
  doCycle();

  // siklus senor 10 menit
  timer.setInterval(SENSOR_PERIOD_MS, doCycle);

  // Cek timer pompa tiap 500 ms (ringan)
  timer.setInterval(500, pumpUpdate);
}

void loop() {
  Blynk.run();
  timer.run();
}
