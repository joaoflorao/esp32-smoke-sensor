#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// Sensors
#define smokeSensor 32
#define smokeSafetyMargin 33
#define BUZZER 25
#define RED_LED 4

// OLED Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Temperature
#define DHTPIN 15
#define DHTTYPE DHT11
#define temperatureSafetyMargin 35
DHT dht(DHTPIN, DHTTYPE);

// WiFi
#define WIFI_SSID "your_wifi_name"
#define WIFI_PASS "your_wifi_password"
#define CONNECTION_ATTEMPTS 5

// Notification
String PHONE_NUMBER = "your_phone_number"; // +international_country_code + phone_number
String API_KEY = "your_api_key";
boolean notificationSent;

// Buzzer
float sine;
int frequency;

// try to connect or continue without wifi connection
void wifiConnect(){
  if (WiFi.status() == WL_CONNECTED || WiFi.status() == WL_IDLE_STATUS) {
    return;
  }

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < CONNECTION_ATTEMPTS) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
}

int sendMessage(String message) {
  if (WiFi.status() != WL_CONNECTED){
    wifiConnect();
  }

  // Data to send with HTTP POST
  String url = "https://api.callmebot.com/whatsapp.php?phone=" + PHONE_NUMBER + "&apikey=" + API_KEY + "&text=" + urlEncode(message);
  HTTPClient http;
  http.begin(url);

  // Send HTTP request
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200) {
    Serial.println("Message sent successfully");
  } else {
    Serial.println("Error sending message");
    Serial.print("HTTP response code: ");
    Serial.println(httpResponseCode);
  }
  http.end();

  return httpResponseCode;
}

void buzzerAlert(int multiplicator){
  for (int x = 0; x < 180; x++){
    sine = sin(x * 3.1416 * multiplicator / 180);
    frequency = 2000 +(int(sine * 1000));
    tone(BUZZER, frequency);

    if (frequency > 2500) {
      digitalWrite(RED_LED, HIGH);
    } else {
      digitalWrite(RED_LED, LOW);
    }

    delay(2);
  }
  noTone(BUZZER);
}

void setup() {
  Serial.begin(115200);

  pinMode(smokeSensor, INPUT);
  pinMode(smokeSafetyMargin, INPUT);
  pinMode(temperatureSafetyMargin, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  // Oled Display
  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Failed to start OLED display.");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Starting...");
  display.display();

  // Temperature Sensor
  dht.begin();

  wifiConnect();
  notificationSent = false;
}

void loop() {
  int smokeSensorValue = analogRead(smokeSensor);
  int smokeMargin = analogRead(smokeSafetyMargin);
  float temperatureMargin = analogRead(temperatureSafetyMargin);

  // Map input values
  int smokeMarginValue = map(smokeMargin, 0, 4095, 200, 2000);
  float temperatureMarginValue = map(temperatureMargin, 0, 4095, 10, 70);

  float temperatureValue = dht.readTemperature(); // Celsius
  float humidityValue = dht.readHumidity();

  display.clearDisplay();

  // Safety Margin
  display.setCursor(0, 0);
  display.print("Smoke Margin: ");
  display.println(smokeMarginValue);

  // Smoke Level
  display.setCursor(0, 10);
  display.print("Smoke Level: ");
  display.println(smokeSensorValue);

  // Temperature Margin
  display.setCursor(0, 30);
  display.print("Temp Margin: ");
  display.print(temperatureMarginValue);
  display.println(" C");

  // Temperature
  display.setCursor(0, 40);
  display.print("Temp: ");
  display.print(temperatureValue);
  display.println(" C");

  display.display();
  if (smokeSensorValue > smokeMarginValue){
    for (int i = 0; i < 4; i++) {
      buzzerAlert(1);
      delay(2);
      buzzerAlert(10);
    }
    if (!notificationSent){
      int httpResponseCode = sendMessage("ATENÇÃO! Gás ou Fumaça detectados!");
      if (httpResponseCode == 200){
        notificationSent = true;
      }
    }
  }
  else if (temperatureValue > temperatureMarginValue){
    for (int i = 0; i < 2; i++) {
      buzzerAlert(2);
      delay(2);
    }
    if (!notificationSent){
      int httpResponseCode = sendMessage("ATENÇÃO! A temperatura está elevada!");
      if (httpResponseCode == 200){
        notificationSent = true;
      }
    }
  }
  else {
    notificationSent = false;
    noTone(BUZZER);
    digitalWrite(RED_LED, LOW);
  }

  delay(2000);
}