#ifndef CONFIG_H
#define CONFIG_H

#define BLYNK_TEMPLATE_ID           "TMPL6WC99jfYu"
#define BLYNK_TEMPLATE_NAME         "Quickstart Template"
#define BLYNK_AUTH_TOKEN            "e4rzTK7W2x6c7klI-l7ZtGl-mydqGQGy"

#define BLYNK_PRINT Serial

#include <Arduino.h>

// DHT11
#define DHT_PIN         D4
#define DHT_TYPE        DHT11

// OLED I2C
#define LCD_PIN_SDA     D2
#define LCD_PIN_SCL     D1

// Relay
#define FAN_RELAY_PIN   D6    // IN1 -> Kipas
#define PUMP_RELAY_PIN  D5    // IN2 -> Pompa

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define OLED_ADDRESS    0x3C

const float TEMP_THRESHOLD = 30.0;   // Kipas ON jika suhu > 30°C
const float HUM_THRESHOLD  = 60.0;   // Pompa ON jika kelembapan < 60%

/* ==========================================================
   Blynk Virtual Pin
   V4  -> Suhu            (Gauge)
   V5  -> Kelembapan      (Gauge)
   V6  -> Wifi            (LED)
   V7  -> Status Kipas    (LED)
   V8  -> Status Pompa    (LED)
   V9  -> Switch Pompa    (Switch, manual ON/OFF)
   V10 -> Switch Kipas    (Switch, manual ON/OFF)
   ========================================================== */

#define VPIN_SUHU       V4
#define VPIN_HUMID      V5
#define VPIN_WIFI       V6
#define VPIN_FAN_LED    V7
#define VPIN_PUMP_LED   V8
#define VPIN_PUMP_SW    V9
#define VPIN_FAN_SW     V10

// Interval
const unsigned long SENSOR_CHECK_INTERVAL = 2000;   // cek sensor & kontrol relay tiap 2 detik
const unsigned long HEARTBEAT_INTERVAL    = 60000;  // paksa kirim data tiap 60 detik walau tidak berubah
const float TEMP_DELTA_MIN  = 0.3;   // kirim suhu ke Blynk hanya jika berubah >= 0.3 C
const float HUM_DELTA_MIN   = 1.0;   // kirim kelembapan ke Blynk hanya jika berubah >= 1 %

extern char ssid[];
extern char pass[];

#endif // CONFIG_H
