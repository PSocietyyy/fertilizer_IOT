#include "hardware.h"
#include "globals.h"
#include <Wire.h>

/* ==========================================================
   Inisialisasi OLED 
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
   Baca Sensor DHT11
   ========================================================== */
void readSensor(float &temperature, float &humidity)
{
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
}

/* ==========================================================
   Kontrol Relay
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
   Display OLED
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
