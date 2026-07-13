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

    // Jalankan pertama kali agar data sensor dan OLED langsung terisi
    checkSensorAndControl();

    Serial.println("System Ready");
}

void loop()
{
    Blynk.run();
    timer.run();
}

/* ==========================================================
   Saat device baru connect ke Blynk:
   Reset status switch manual ke OFF (Auto), matikan semua LED status,
   dan kirim status Wifi.
   ========================================================== */
BLYNK_CONNECTED()
{
    Blynk.virtualWrite(VPIN_PUMP_SW, 0);
    Blynk.virtualWrite(VPIN_PUMP_LED, 0);
    Blynk.virtualWrite(VPIN_FAN_LED, 0);
    Blynk.virtualWrite(VPIN_WIFI, 1);

    lastSentPumpState = false;
    lastSentFanState = false;
    lastSentWifi = 1;
}



/* ==========================================================
   Cek sensor, jalankan kontrol relay, update OLED,
   dan kirim ke Blynk HANYA kalau perlu (hemat kuota)
   ========================================================== */
void checkSensorAndControl()
{
    readSensor(currentTemp, currentHum);

    if (isnan(currentTemp) || isnan(currentHum))
    {
        Serial.println("Failed to read DHT11");
        showError("DHT11 ERROR");
        return;
    }

    controlRelay(currentTemp, currentHum);

    bool fanState  = (digitalRead(FAN_RELAY_PIN)  == HIGH);
    bool pumpState = (digitalRead(PUMP_RELAY_PIN) == HIGH);

    Serial.println("--------------------------------");
    Serial.printf("Temperature : %.1f C\n", currentTemp);
    Serial.printf("Humidity    : %.1f %%\n", currentHum);
    Serial.printf("Fan         : %s (auto)\n", fanState ? "ON" : "OFF");
    Serial.printf("Pump        : %s (auto)\n", pumpState ? "ON" : "OFF");

    showSensorData(currentTemp, currentHum);

    bool needSend = false;
    unsigned long now = millis();

    // Heartbeat: paksa kirim tiap 60 detik walau tidak ada perubahan
    if (now - lastHeartbeat >= HEARTBEAT_INTERVAL)
    {
        needSend = true;
        lastHeartbeat = now;
    }

    // Kirim suhu hanya jika berubah cukup signifikan
    if (fabs(currentTemp - lastSentTemp) >= TEMP_DELTA_MIN || needSend)
    {
        Blynk.virtualWrite(VPIN_SUHU, currentTemp);
        lastSentTemp = currentTemp;
    }

    // Kirim kelembapan hanya jika berubah cukup signifikan
    if (fabs(currentHum - lastSentHum) >= HUM_DELTA_MIN || needSend)
    {
        Blynk.virtualWrite(VPIN_HUMID, currentHum);
        lastSentHum = currentHum;
    }

    // Kirim status kipas hanya jika berubah
    if (fanState != lastSentFanState || needSend)
    {
        Blynk.virtualWrite(VPIN_FAN_LED, fanState ? 1 : 0);
        lastSentFanState = fanState;
    }

    // Kirim status pompa dan switch Blynk hanya jika berubah
    if (pumpState != lastSentPumpState || needSend)
    {
        Blynk.virtualWrite(VPIN_PUMP_SW, pumpState ? 1 : 0);
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