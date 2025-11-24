//MY DAQ (2021t01087: D D Athukorala)
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>

// Pin Definitions
#define DHTPIN 27
#define DHTTYPE DHT11
#define TdsSensorPin 35
#define soilMoisturePin 34

// Constants
#define VREF 3.3 //reference voltage ADC
#define SCOUNT 30 //sample count

// Global Variables
float roomTemp = 0;
float ds18b20Temp = 0;
float humidity = 0;
float tdsValue = 0;
float soilMoistureValue = 0;

// Pin Definitions for Buzzer
#define BUZZER_PIN 23

// Pin Definitions for LEDs
#define LED_HUMIDITY 4
#define LED_WATER_TEMP 5
#define LED_SOIL_MOISTURE 18
#define LED_TDS 19
#define LED_REAL_TIME_MODE 22

const char* ssid = "DUNITH";
const char* password = "123456789";

OneWire oneWire(25);
DallasTemperature sensors(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80); //port number HTTP REQUEST

bool manualMode = false;

void setup() {
  Serial.begin(115200);

  dht.begin();

  //buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Configure LED pins as output
  pinMode(LED_HUMIDITY, OUTPUT);
  pinMode(LED_WATER_TEMP, OUTPUT);
  pinMode(LED_TDS, OUTPUT);
  pinMode(LED_SOIL_MOISTURE, OUTPUT);
  pinMode(LED_REAL_TIME_MODE, OUTPUT);

  // Ensure LED and buzzer are OFF initially
  digitalWrite(BUZZER_PIN, LOW);
  
  // Initialize LEDs to OFF state
  digitalWrite(LED_HUMIDITY, LOW);
  digitalWrite(LED_WATER_TEMP, LOW);
  digitalWrite(LED_TDS, LOW);
  digitalWrite(LED_SOIL_MOISTURE, LOW);
  digitalWrite(LED_REAL_TIME_MODE, LOW);
  
  pinMode(TdsSensorPin, INPUT);
  pinMode(soilMoisturePin, INPUT);
  
  sensors.begin();
  dht.begin();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("Web server IP address: ");
  Serial.println(WiFi.localIP());

  // Define web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData); // Real-time data endpoint
  server.begin();
}

void loop() {
  static unsigned long lastUpdate = millis();

  digitalWrite(LED_REAL_TIME_MODE, !manualMode ? HIGH : LOW);
  
  if (!manualMode && millis() - lastUpdate > 2000) { // Update every 2 seconds
    lastUpdate = millis();
    updateSensorData();
  }

  server.handleClient();
}

void updateSensorData() {
  // Read DS18B20 temperature (Water Temp)
  sensors.requestTemperatures();
  ds18b20Temp = sensors.getTempCByIndex(0);

  // Read DHT11
  roomTemp = dht.readTemperature();
  humidity = dht.readHumidity();

  // Process TDS value
  float voltage = analogRead(TdsSensorPin) * VREF / 4095.0;
  float compensationCoefficient = 1.0 + 0.02 * (ds18b20Temp - 25.0);
  float compensationVoltage = voltage / compensationCoefficient;
  tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage 
             - 255.86 * compensationVoltage * compensationVoltage 
             + 857.39 * compensationVoltage) * 0.5;

  // Read soil moisture
  int sensorValue = analogRead(soilMoisturePin);  // Read raw analog value
  float percentage = map(sensorValue, 4095, 0, 0, 100);  // Map to percentage (0–100)
  soilMoistureValue = percentage; // Update the global variable

  // Control LEDs based on parameter ranges
  digitalWrite(BUZZER_PIN, (roomTemp < 15 || roomTemp > 30) ? HIGH : LOW); // Example range: 15-30 °C
  digitalWrite(LED_HUMIDITY, (humidity < 40 || humidity > 60) ? HIGH : LOW); // Example range: 40-60 %
  digitalWrite(LED_WATER_TEMP, (ds18b20Temp < 10 || ds18b20Temp > 35) ? HIGH : LOW); // Example range: 10-35 °C
  digitalWrite(LED_SOIL_MOISTURE, (soilMoistureValue < 30 || soilMoistureValue > 70) ? HIGH : LOW); // Example range: 30-70 %
  digitalWrite(LED_TDS, (tdsValue < 500 || tdsValue > 1500) ? HIGH : LOW); // Example range: 500-1500 ppm
}


void handleRoot() {
  String html = "<!DOCTYPE html><html lang='en'><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Smart Home Garden Dashboard</title>";
  html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>"; // Include Chart.js
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #f0f4f7; color: #333; }";
  html += ".container { max-width: 1000px; margin: 20px auto; padding: 20px; text-align: center; }";
  html += "h1 { font-size: 2.5rem; color: #2c6b2f; }";  // Green for title
  html += "h1 { font-size: 2.5rem; color: #2c6b2f; }";
  html += "canvas { max-width: 100%; height: 400px; }";
  // Flexbox Layout and Media Queries for Responsiveness
  html += ".event-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; }";
  html += ".event-grid-2 { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin-top: 20px; }";

  html += ".event-box { padding: 30px; border-radius: 15px; box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1); transition: transform 0.3s ease; background: linear-gradient(135deg, #70c8a7 0%, #34a853 100%); color: #fff; }";  // Soft green background
  html += ".event-box:hover { transform: scale(1.05); }";
  html += ".event-box h3 { margin-bottom: 10px; font-size: 1.2rem; }";
  html += ".event-box .inner { font-size: 2rem; font-weight: bold; }";
  html += "#toggleMode { margin-top: 30px; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; background-color: #2c6b2f; color: #fff; font-size: 1rem; transition: background 0.3s; }";  // Darker green for button
  html += "#toggleMode:hover { background-color: #388e3c; }";  // Darker on hover
  html += ".footer { margin-top: 30px; font-size: 0.9rem; color: #777; }";
  html += "</style></head><body>";

  html += "<div class='container'>";
  html += "<h1>GreenHeaven Monitor <br/>Smart Home Garden Dashboard</h1>";
  
  html += "<div class='event-grid'>";

  html += "<div class='event-box'>";
  html += "<img src='https://img.icons8.com/?size=100&id=obVKDjvQfjSg&format=png&color=000000' alt='Temperature'>";
  html += "<div class='progress-bar'><div id='progress-roomTemp' style='width: 0%;'></div></div>";
  html += "<h3>Room Temperature</h3>";
  html += "<div class='inner' id='eventBox-roomTemp'>-- °C</div>";
  html += "</div>";

  html += "<div class='event-box'>";
  html += "<img src='https://img.icons8.com/?size=100&id=Pqc7k1Yn5daR&format=png&color=000000' alt='Humidity'>";
  html += "<div class='progress-bar'><div id='progress-humidity' style='width: 0%;'></div></div>";
  html += "<h3>Humidity</h3>";
  html += "<div class='inner' id='eventBox-humidity'>-- %</div>";
  html += "</div>";

  html += "<div class='event-box'>";
  html += "<img src='https://img.icons8.com/?size=100&id=44932&format=png&color=000000' alt='Water Temp'>";
  html += "<div class='progress-bar'><div id='progress-ds18b20' style='width: 0%;'></div></div>";
  html += "<h3>Water Temperature</h3>";
  html += "<div class='inner' id='eventBox-ds18b20'>-- °C</div></div>";
  html += "</div>";
  

  html += "<div class='event-grid-2'>";

  html += "<div class='event-box'>";
  html += "<img src='https://img.icons8.com/?size=100&id=WdpOiowt42I7&format=png&color=000000' alt='Soil Moisture'>";
  html += "<div class='progress-bar'><div id='progress-soilMoisture' style='width: 0%;'></div></div>";
  html += "<h3>Soil Moisture</h3>";
  html += "<div class='inner' id='eventBox-soilMoisture'>-- %</div>";
  html += "</div>";

  html += "<div class='event-box'>";
  html += "<img src='https://img.icons8.com/?size=100&id=x2ITGUL3I9n5&format=png&color=000000' alt='Temperature'>";
  html += "<div class='progress-bar'><div id='progress-tds' style='width: 0%;'></div></div>";
  html +="<h3>TDS Level</h3>";
  html +="<div class='inner' id='eventBox-tds'>-- ppm</div>";
  html +="</div>";
  html += "</div>";

  html += "<script>";
  html += "function updateProgressBar(id, value, max) {";
  html += "  const bar = document.getElementById(id);";
  html += "  const percentage = (value / max) * 100;";
  html += "  bar.style.width = percentage + '%';";
  html += "}";
  html += "</script>";
  
  html += "<button id='toggleMode' onclick='toggleMode()'>Switch to Manual Mode</button>";

  html += "<div class='footer'>GreenHeaven Monitor(MY DAQ) | Designed by Dunith</div>";
  html += "</div>";

  html += "<div class='container'>";
  html += "<script>";
  html += "let manualMode = false;";
  html += "function toggleMode() {";
  html += "  manualMode = !manualMode;";
  html += "  document.getElementById('toggleMode').innerText = manualMode ? 'Switch to Real-Time Mode' : 'Switch to Manual Mode';";
  html += "}";

  html += "function updateEventBoxes() {";
  html += "  if (!manualMode) {";
  html += "    fetch('/data').then(response => response.json()).then(data => {";
  html += "      document.querySelector('#eventBox-tds').textContent = data.tds + ' ppm';";
  html += "      document.querySelector('#eventBox-roomTemp').textContent = data.roomTemp + ' °C';";
  html += "      document.querySelector('#eventBox-ds18b20').textContent = data.ds18b20 + ' °C';";
  html += "      document.querySelector('#eventBox-humidity').textContent = data.humidity + ' %';";
  html += "      document.querySelector('#eventBox-soilMoisture').textContent = data.soilMoisture + ' %';";
  html += "    });";
  html += "  }";
  html += "}";
  html += "setInterval(updateEventBoxes, 2000);";
  html += "</script></body></html>";


  html += "<div class='container'>";
  html += "<h1>Smart Home Garden Graph View</h1>";

  html += "<div><canvas id='roomTempChart'></canvas></div>";
  html += "<div><canvas id='humidityChart'></canvas></div>";
  html += "<div><canvas id='ds18b20Chart'></canvas></div>";
  html += "<div><canvas id='tdsChart'></canvas></div>";
  html += "<div><canvas id='soilMoistureChart'></canvas></div>";

  html += "</div>";

  html += "<script>";
  html += "let roomTempData = [], humidityData = [], ds18b20Data =[], tdsData = [], soilMoistureData = [];";
  html += "let timeLabels = [];";
  html += "const maxDataPoints = 10;"; // Limit chart data points

  // Chart initialization
  // Room Temperature Chart
  html += "const roomTempChart = new Chart(document.getElementById('roomTempChart').getContext('2d'), {";
  html += "  type: 'line', data: { labels: timeLabels, datasets: [{ label: 'Room Temperature (°C)', data: roomTempData, borderColor: 'red', fill: false }] }, options: { scales: { x: { beginAtZero: false }, y: { beginAtZero: true } } }";
  html += "});";

  // Humidity Chart
  html += "const humidityChart = new Chart(document.getElementById('humidityChart').getContext('2d'), {";
  html += "  type: 'line', data: { labels: timeLabels, datasets: [{ label: 'Humidity (%)', data: humidityData, borderColor: 'blue', fill: false }] }, options: { scales: { x: { beginAtZero: false }, y: { beginAtZero: true } } }";
  html += "});";

  // Water Temperature Chart
  html += "const ds18b20Chart = new Chart(document.getElementById('ds18b20Chart').getContext('2d'), {";
  html += "  type: 'line', data: { labels: timeLabels, datasets: [{ label: 'Water Temperature (°C)', data: ds18b20Data, borderColor: 'purple', fill: false }] }, options: { scales: { x: { beginAtZero: false }, y: { beginAtZero: true } } }";
  html += "});";

  // Soil Moisture Chart
  html += "const soilMoistureChart = new Chart(document.getElementById('soilMoistureChart').getContext('2d'), {";
  html += "  type: 'line', data: { labels: timeLabels, datasets: [{ label: 'Soil Moisture (%)', data: soilMoistureData, borderColor: 'orange', fill: false }] }, options: { scales: { x: { beginAtZero: false }, y: { beginAtZero: true } } }";
  html += "});";

  // TDS Chart
  html += "const tdsChart = new Chart(document.getElementById('tdsChart').getContext('2d'), {";
  html += "  type: 'line', data: { labels: timeLabels, datasets: [{ label: 'TDS (ppm)', data: tdsData, borderColor: 'green', fill: false }] }, options: { scales: { x: { beginAtZero: false }, y: { beginAtZero: true } } }";
  html += "});";

  // Update charts
  html += "function updateCharts() {";
  html += "  fetch('/data').then(response => response.json()).then(data => {";
  html += "    const now = new Date().toLocaleTimeString();";
  html += "    if (timeLabels.length >= maxDataPoints) timeLabels.shift();";
  html += "    timeLabels.push(now);";

  html += "    if (roomTempData.length >= maxDataPoints) roomTempData.shift();";
  html += "    roomTempData.push(data.roomTemp);";

  html += "    if (humidityData.length >= maxDataPoints) humidityData.shift();";
  html += "    humidityData.push(data.humidity);";

  html += "    if (ds18b20Data.length >= maxDataPoints) ds18b20Data.shift();"; // Add Water Temp
  html += "    ds18b20Data.push(data.ds18b20);"; // Push Water Temp Data


  html += "    if (tdsData.length >= maxDataPoints) tdsData.shift();";
  html += "    tdsData.push(data.tds);";

  html += "    if (soilMoistureData.length >= maxDataPoints) soilMoistureData.shift();";
  html += "    soilMoistureData.push(data.soilMoisture);";

  // Update Charts
  html += "    roomTempChart.update();";
  html += "    humidityChart.update();";
  html += "    ds18b20Chart.update();";
  html += "    tdsChart.update();";
  html += "    soilMoistureChart.update();";
  html += "  });";
  html += "}";

  html += "setInterval(updateCharts, 2000);"; // Fetch data every 2 seconds
  html += "</script></body></html>";

  html += "<div class='footer'>GreenHeaven Monitor(MY DAQ) | Designed by Dunith</div>";
  html += "</div>";

  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"tds\":" + String(tdsValue, 0) + ",";
  json += "\"roomTemp\":" + String(roomTemp, 1) + ",";
  json += "\"ds18b20\":" + String(ds18b20Temp, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"soilMoisture\":" + String(soilMoistureValue, 1);
  json += "}";
  server.send(200, "application/json", json);
}


