#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "globals.h"
#include <BlynkSimpleEsp8266.h>
#include "hardware.h"

void checkSensorAndControl();

void setup()
{
    Serial.begin(115200);
    Serial.println();

    dht.begin();

    // Login ke Blynk Cloud
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

    pinMode(FAN_RELAY_PIN, OUTPUT);
    pinMode(PUMP_RELAY_PIN, OUTPUT);

    // Relay OFF (Active LOW)
    fanOff();
    pumpOff();

    initOLED();

    // Cek sensor tiap 2 detik, tapi kirim ke Blynk cuman kalau perlu (checkSensorAndControl)
    timer.setInterval(SENSOR_CHECK_INTERVAL, checkSensorAndControl);

    Serial.println("System Ready");
}

void loop()
{
    Blynk.run();
    timer.run();
}

/* ==========================================================
   Saat device baru connect ke Blynk:
   sinkronkan status switch manual (biar kalau ESP restart,
   posisi switch di app tetap dipakai) dan kirim status Wifi.
   ========================================================== */
BLYNK_CONNECTED()
{
    Blynk.syncVirtual(VPIN_FAN_SW, VPIN_PUMP_SW);
    Blynk.virtualWrite(VPIN_WIFI, 1);
    lastSentWifi = 1;
}

/* ==========================================================
   Switch manual dari Blynk
   ========================================================== */
BLYNK_WRITE(VPIN_FAN_SW)
{
    // ON  -> bypass, paksa kipas nyala walau suhu belum melewati threshold
    // OFF -> lepas bypass, kipas kembali mengikuti kontrol otomatis (bukan dipaksa mati)
    fanBypass = param.asInt();
}

BLYNK_WRITE(VPIN_PUMP_SW)
{
    // ON  -> bypass, paksa pompa nyala walau kelembapan belum di bawah threshold
    // OFF -> lepas bypass, pompa kembali mengikuti kontrol otomatis (bukan dipaksa mati)
    pumpBypass = param.asInt();
}

/* ==========================================================
   Cek sensor, jalankan kontrol relay, update OLED,
   dan kirim ke Blynk HANYA kalau perlu (hemat kuota)
   ========================================================== */
void checkSensorAndControl()
{
    float temperature;
    float humidity;

    readSensor(temperature, humidity);

    if (isnan(temperature) || isnan(humidity))
    {
        Serial.println("Failed to read DHT11");
        showError("DHT11 ERROR");
        return;
    }

    controlRelay(temperature, humidity);

    bool fanState  = (digitalRead(FAN_RELAY_PIN)  == LOW);
    bool pumpState = (digitalRead(PUMP_RELAY_PIN) == LOW);

    Serial.println("--------------------------------");
    Serial.printf("Temperature : %.1f C\n", temperature);
    Serial.printf("Humidity    : %.1f %%\n", humidity);
    Serial.printf("Fan         : %s (%s)\n", fanState ? "ON" : "OFF", fanBypass ? "bypass" : "auto");
    Serial.printf("Pump        : %s (%s)\n", pumpState ? "ON" : "OFF", pumpBypass ? "bypass" : "auto");

    showSensorData(temperature, humidity);

    bool needSend = false;
    unsigned long now = millis();

    // Heartbeat: paksa kirim tiap 60 detik walau tidak ada perubahan
    if (now - lastHeartbeat >= HEARTBEAT_INTERVAL)
    {
        needSend = true;
        lastHeartbeat = now;
    }

    // Kirim suhu hanya jika berubah cukup signifikan
    if (fabs(temperature - lastSentTemp) >= TEMP_DELTA_MIN || needSend)
    {
        Blynk.virtualWrite(VPIN_SUHU, temperature);
        lastSentTemp = temperature;
    }

    // Kirim kelembapan hanya jika berubah cukup signifikan
    if (fabs(humidity - lastSentHum) >= HUM_DELTA_MIN || needSend)
    {
        Blynk.virtualWrite(VPIN_HUMID, humidity);
        lastSentHum = humidity;
    }

    // Kirim status kipas hanya jika berubah
    if (fanState != lastSentFanState || needSend)
    {
        Blynk.virtualWrite(VPIN_FAN_LED, fanState ? 1 : 0);
        lastSentFanState = fanState;
    }

    // Kirim status pompa hanya jika berubah
    if (pumpState != lastSentPumpState || needSend)
    {
        Blynk.virtualWrite(VPIN_PUMP_LED, pumpState ? 1 : 0);
        lastSentPumpState = pumpState;
    }

    // Status Wifi cukup dicek/dikirim saat heartbeat saja
    if (needSend)
    {
        int wifiNow = (WiFi.status() == WL_CONNECTED) ? 1 : 0;
        if (wifiNow != lastSentWifi)
        {
            Blynk.virtualWrite(VPIN_WIFI, wifiNow);
            lastSentWifi = wifiNow;
        }
    }
}