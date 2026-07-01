#define BLYNK_TEMPLATE_ID           "TMPL6WC99jfYu"
#define BLYNK_TEMPLATE_NAME         "Quickstart Template"
#define BLYNK_AUTH_TOKEN            "e4rzTK7W2x6c7klI-l7ZtGl-mydqGQGy"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* ==========================================================
   Pin Configuration (fisik ESP8266)
   ========================================================== */

// DHT11
#define DHT_PIN         D4
#define DHT_TYPE        DHT11

// OLED I2C
#define LCD_PIN_SDA     D2
#define LCD_PIN_SCL     D1

// Relay
#define FAN_RELAY_PIN   D6    // IN1 -> Kipas
#define PUMP_RELAY_PIN  D5    // IN2 -> Pompa

/* ==========================================================
   OLED Configuration
   ========================================================== */

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define OLED_ADDRESS    0x3C

/* ==========================================================
   Threshold otomatis
   ========================================================== */

const float TEMP_THRESHOLD = 30.0;   // Kipas ON jika suhu > 30°C
const float HUM_THRESHOLD  = 60.0;   // Pompa ON jika kelembapan < 60%

/* ==========================================================
   Blynk Virtual Pin Mapping (sesuai dashboard, tanpa Uptime)
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

/* ==========================================================
   Interval (hemat kuota Blynk)
   ========================================================== */

const unsigned long SENSOR_CHECK_INTERVAL = 2000;   // cek sensor & kontrol relay tiap 2 detik
const unsigned long HEARTBEAT_INTERVAL    = 60000;  // paksa kirim data tiap 60 detik walau tidak berubah
const float TEMP_DELTA_MIN  = 0.3;   // kirim suhu ke Blynk hanya jika berubah >= 0.3 C
const float HUM_DELTA_MIN   = 1.0;   // kirim kelembapan ke Blynk hanya jika berubah >= 1 %

/* ==========================================================
   Objects
   ========================================================== */

DHT dht(DHT_PIN, DHT_TYPE);

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    OLED_RESET
);

BlynkTimer timer;

/* ==========================================================
   WiFi Configuration
   ========================================================== */
char ssid[] = "IPhone Perdi";
char pass[] = "12345678999";

/* ==========================================================
   State
   ========================================================== */

bool  fanBypass   = false;   // true = switch di app ON -> paksa kipas nyala (bypass sensor)
bool  pumpBypass  = false;   // true = switch di app ON -> paksa pompa nyala (bypass sensor)

float lastSentTemp     = -999;
float lastSentHum      = -999;
bool  lastSentFanState = false;
bool  lastSentPumpState= false;
int   lastSentWifi     = -1;

unsigned long lastHeartbeat = 0;

/* ==========================================================
   Function Prototype
   ========================================================== */

void initOLED();
void readSensor(float &temperature, float &humidity);
void showSensorData(float temperature, float humidity);
void showError(const char *message);

void controlRelay(float temperature, float humidity);

void fanOn();
void fanOff();

void pumpOn();
void pumpOff();

void checkSensorAndControl();

/* ==========================================================
   Setup
   ========================================================== */

void setup()
{
    Serial.begin(115200);
    Serial.println();

    dht.begin();

    // Blynk.begin akan menyambungkan WiFi sekaligus login ke Blynk Cloud
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

    pinMode(FAN_RELAY_PIN, OUTPUT);
    pinMode(PUMP_RELAY_PIN, OUTPUT);

    // Relay OFF (Active LOW)
    fanOff();
    pumpOff();

    initOLED();

    // Cek sensor tiap 2 detik, tapi kirim ke Blynk hanya kalau perlu (lihat checkSensorAndControl)
    timer.setInterval(SENSOR_CHECK_INTERVAL, checkSensorAndControl);

    Serial.println("System Ready");
}

/* ==========================================================
   Loop
   ========================================================== */

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
   Switch manual dari app Blynk
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

/* ==========================================================
   OLED Initialization
   ========================================================== */

void initOLED()
{
    Wire.begin(LCD_PIN_SDA, LCD_PIN_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS))
    {
        Serial.println("OLED initialization failed!");

        while (true);
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(8, 10);
    display.println("ESP8266");

    display.setTextSize(1);
    display.setCursor(18, 40);
    display.println("OLED Ready");

    display.display();

    delay(1500);
}

/* ==========================================================
   Read Sensor
   ========================================================== */

void readSensor(float &temperature, float &humidity)
{
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
}

/* ==========================================================
   Relay Control
   ========================================================== */

void controlRelay(float temperature, float humidity)
{
    // Kipas
    if (fanBypass)
    {
        // Switch di app sedang ON -> paksa nyala, abaikan suhu
        fanOn();
    }
    else
    {
        // Mode otomatis berdasarkan suhu
        if (temperature > TEMP_THRESHOLD)
            fanOn();
        else
            fanOff();
    }

    // Pompa
    if (pumpBypass)
    {
        // Switch di app sedang ON -> paksa nyala, abaikan kelembapan
        pumpOn();
    }
    else
    {
        // Mode otomatis berdasarkan kelembapan
        if (humidity < HUM_THRESHOLD)
            pumpOn();
        else
            pumpOff();
    }
}

void fanOn()
{
    digitalWrite(FAN_RELAY_PIN, LOW);     // Active LOW
}

void fanOff()
{
    digitalWrite(FAN_RELAY_PIN, HIGH);
}

void pumpOn()
{
    digitalWrite(PUMP_RELAY_PIN, LOW);    // Active LOW
}

void pumpOff()
{
    digitalWrite(PUMP_RELAY_PIN, HIGH);
}

/* ==========================================================
   OLED Display
   ========================================================== */

void showSensorData(float temperature, float humidity)
{
    display.clearDisplay();

    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("Monitoring DHT11");

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display.setCursor(0, 18);
    display.print("Temp : ");
    display.print(temperature, 1);
    display.print(" C");

    display.setCursor(0, 32);
    display.print("Hum  : ");
    display.print(humidity, 1);
    display.print(" %");

    display.setCursor(0, 50);
    display.print("Fan:");
    display.print(digitalRead(FAN_RELAY_PIN) == LOW ? "ON " : "OFF");

    display.print(" Pump:");
    display.print(digitalRead(PUMP_RELAY_PIN) == LOW ? "ON" : "OFF");

    display.display();
}

/* ==========================================================
   Error Display
   ========================================================== */

void showError(const char *message)
{
    display.clearDisplay();

    display.setTextSize(2);
    display.setCursor(0, 15);
    display.println("ERROR");

    display.setTextSize(1);
    display.setCursor(0, 45);
    display.println(message);

    display.display();
}