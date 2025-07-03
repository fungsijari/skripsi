#define BLYNK_TEMPLATE_ID "TMPL63ulhcZ2x"
#define BLYNK_TEMPLATE_NAME "SKRIPSI"
#define BLYNK_AUTH_TOKEN "esHuhtZehgv3HnYu1gKlY_ePBZhNUOkf"
#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ModbusMaster.h>
#include <SoftwareSerial.h>

// Konfigurasi pin
#define DE_PIN 18
#define RE_PIN 19
#define RX_PIN 16
#define TX_PIN 17
#define POMPA_PIN 5
#define SLAVE_ADDR 0x01

// WiFi credentials
char ssid[] = "F25";
char pass[] = "bayu9285";

// Objek Modbus dan serial
ModbusMaster node;
SoftwareSerial mySerial(RX_PIN, TX_PIN);

// Variabel data
uint16_t nilai;
float ph, kel, t;

void preTransmission() {
  digitalWrite(DE_PIN, HIGH);
  digitalWrite(RE_PIN, HIGH);
}

void postTransmission() {
  digitalWrite(DE_PIN, LOW);
  digitalWrite(RE_PIN, LOW);
}

void setup() {
  // Konfigurasi pin kontrol
  pinMode(DE_PIN, OUTPUT);
  pinMode(RE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);
  digitalWrite(RE_PIN, LOW);

  pinMode(POMPA_PIN, OUTPUT);
  digitalWrite(POMPA_PIN, LOW);  // Relay mati (karena aktif HIGH)

  // Inisialisasi komunikasi
  Serial.begin(9600);
  mySerial.begin(4800);

  // Inisialisasi Modbus
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  node.begin(SLAVE_ADDR, mySerial);

  // Koneksi Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

// Fungsi fuzzy logic
float fuzzyPump(float kelembapan) {
  float dry = 0, moist = 0, veryWet = 0;

  // Dry
  if (kelembapan < 20)
    dry = 1;
  else if (kelembapan >= 20 && kelembapan <= 50)
    dry = (50 - kelembapan) / 30.0;
  else
    dry = 0;

  // Moist
  if (kelembapan >= 50 && kelembapan <= 60)
    moist = (kelembapan - 50) / 10.0;
  else if (kelembapan > 60 && kelembapan <= 70)
    moist = (70 - kelembapan) / 10.0;
  else
    moist = 0;

  // Very Wet
  if (kelembapan >= 70 && kelembapan <= 85)
    veryWet = (kelembapan - 70) / 15.0;
  else if (kelembapan > 85)
    veryWet = 1;
  else
    veryWet = 0;

  // Defuzzifikasi
  float z_fast = 100;
  float z_medium = 60;
  float z_slow = 20;

  float numerator = dry * z_fast + moist * z_medium + veryWet * z_slow;
  float denominator = dry + moist + veryWet;

  if (denominator == 0) return 0;
  return numerator / denominator;
}

// Fungsi baca sensor
int baca_sensor(uint16_t alamat) {
  int result = node.readHoldingRegisters(alamat, 1);
  if (result == node.ku8MBSuccess) {
    nilai = node.getResponseBuffer(0);
  } else {
    Serial.println("Modbus gagal membaca alamat " + String(alamat));
    nilai = 0;
  }
  return nilai;
}

void loop() {
  Blynk.run();

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

  // Logika pompa (relay aktif HIGH)
  if (kel < 50) {
    digitalWrite(POMPA_PIN, HIGH);   // Pompa HIDUP
    Serial.println("Pompa: ON");
  } else {
    digitalWrite(POMPA_PIN, LOW);    // Pompa MATI
    Serial.println("Pompa: OFF");
  }

  delay(1000);
}
