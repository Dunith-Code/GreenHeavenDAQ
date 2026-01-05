/********************************************************************
 *  GreenHeaven
 *  Group 01
 ********************************************************************/

#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ------------------- INPUT PINS (3.3V ONLY) -------------------
const int DHT_PIN      = 27;   // DHT11
const int ONE_WIRE_BUS = 25;   // DS18B20 (Water Temp)
const int TDS_PIN      = 35;   // TDS Analog
const int SOIL_PIN     = 34;   // Soil Moisture Analog

// ------------------- OUTPUT PINS -------------------
const int SOIL_LED_PIN       = 18;
const int DHT_LED_PIN        = 4;
const int WATER_TEMP_LED_PIN = 5;
const int TDS_LED_PIN        = 19;
const int BUZZER_PIN         = 23;
const int POWER_LED_PIN      = 22;   // SYSTEM POWER LED (ON when running)

// ------------------- SENSOR OBJECTS -------------------
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ------------------- TIMER -------------------
unsigned long previousMillis = 0;
const long    interval      = 500;   // 500 ms

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }  // Wait for USB serial

  // ----- OUTPUT PINS -----
  pinMode(SOIL_LED_PIN,       OUTPUT);
  pinMode(DHT_LED_PIN,        OUTPUT);
  pinMode(WATER_TEMP_LED_PIN, OUTPUT);
  pinMode(TDS_LED_PIN,        OUTPUT);
  pinMode(BUZZER_PIN,         OUTPUT);
  pinMode(POWER_LED_PIN,      OUTPUT);

  // ----- TURN ON POWER LED (SYSTEM IS ON) -----
  digitalWrite(POWER_LED_PIN, HIGH);

  // ----- START SENSORS -----
  dht.begin();
  sensors.begin();

  Serial.println("ESP32 Ready | Power LED ON (GPIO 22) | TDS: 3.3V");
}

void loop() {
  /* ---------- 1. RECEIVE COMMANDS FROM LABVIEW ---------- */
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'a': digitalWrite(SOIL_LED_PIN, HIGH); break;
      case 'b': digitalWrite(SOIL_LED_PIN, LOW);  break;
      case 'c': digitalWrite(DHT_LED_PIN, HIGH); break;
      case 'd': digitalWrite(DHT_LED_PIN, LOW);  break;
      case 'e': digitalWrite(WATER_TEMP_LED_PIN, HIGH); break;
      case 'f': digitalWrite(WATER_TEMP_LED_PIN, LOW);  break;
      case 'i': digitalWrite(TDS_LED_PIN, HIGH); break;
      case 'j': digitalWrite(TDS_LED_PIN, LOW);  break;
      case 'g': digitalWrite(BUZZER_PIN, HIGH); break;
      case 'h': digitalWrite(BUZZER_PIN, LOW);  break;
    }
  }

  /* ---------- 2. SEND DATA EVERY 500 ms ---------- */
  unsigned long now = millis();
  if (now - previousMillis >= interval) {
    previousMillis = now;

    // ---- READ DHT11 (Room Temp + Humidity) ----
    float roomTemp = dht.readTemperature();   // °C
    float humidity = dht.readHumidity();      // %RH

    // ---- READ DS18B20 (Water Temp) ----
    sensors.requestTemperatures();
    float waterTemp = sensors.getTempCByIndex(0);
    if (waterTemp == DEVICE_DISCONNECTED_C) waterTemp = -127;

    // ---- READ SOIL (Average 10 readings for stability) ----
    int soilRaw = 0;
    for (int i = 0; i < 10; i++) {
      soilRaw += analogRead(SOIL_PIN);
      delay(1);
    }
    soilRaw /= 10;

    // ---- READ TDS (STABLE + DELAY) ----
    delay(10);  // Let ADC settle
    int tdsRaw = analogRead(TDS_PIN);
    delay(10);

    // ---- ERROR CHECK ----
    if (isnan(roomTemp) || isnan(humidity) || waterTemp == -127) {
      return;  // Skip bad data
    }

    // ---- TDS CONVERSION (Temperature Compensated) ----
    float voltage = tdsRaw * (3.3 / 4095.0);
    float comp = 1.0 + 0.02 * (waterTemp - 25.0);
    float compVolt = (comp != 0) ? voltage / comp : 0;
    float tdsPPM = (133.42 * pow(compVolt, 3)
                  - 255.86 * pow(compVolt, 2)
                  + 857.39 * compVolt) * 0.5;

    // Clamp TDS to realistic range
    if (tdsPPM < 0) tdsPPM = 0;
    if (tdsPPM > 2000) tdsPPM = 2000;

    // ---- SEND DATA IN ORDER ----
    Serial.print(roomTemp, 1);   Serial.print(',');
    Serial.print(humidity, 1);   Serial.print(',');
    Serial.print(waterTemp, 1);  Serial.print(',');
    Serial.print(soilRaw);       Serial.print(',');
    Serial.print(tdsPPM, 1);     Serial.print('\n');  // ← TDS with 1 decimal
  }
}