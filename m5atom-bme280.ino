#include <BME280I2C.h>
#include <Wire.h>
#include <M5Atom.h>
#include <WiFi.h>
#include <Ambient.h>
#include "./conf.h"

#define SERIAL_BAUD (115200)
#define SCK_PIN (19) // SCL
#define SDI_PIN (22) // SDA

enum class AmbientField {
  Temperature = 1,
  Humidity = 2,
  AtmosphericPressure = 3,
};

WiFiClient wifi;
Ambient ambient;
BME280I2C bme;

float toCelsius(const float & f) {
  return (f - 32) * 5 / 9;
}

void setup() {
  // Start M5
  M5.begin(true, false, true);
  delay(50);
  // 赤色に点灯させる
  M5.dis.drawpix(0, 0x00ff00);

  // Start serial
  Serial.begin(SERIAL_BAUD);
  while (!Serial);

  // Start WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Wait for WiFi connection ...");
  }
  Serial.print("Connected WiFi: ");
  Serial.println(WiFi.localIP());

  // Start ambient
  ambient.begin(AMBIENT_CHANNEL_ID, AMBIENT_WRITE_KEY, &wifi);

  // Setup pins
  pinMode(SCK_PIN, INPUT_PULLUP);
  pinMode(SDI_PIN, INPUT_PULLUP);

  // Start I2C
  Wire.begin(SDI_PIN, SCK_PIN);

  // Start BME280
  while (!bme.begin()) {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }
  // chipModelがBME280あれば緑、そうでなければ赤に点灯させる
  M5.dis.drawpix(0, bme.chipModel() == BME280::ChipModel::ChipModel_BME280 ? 0xff0000 : 0x00ff00); //GRB

}

void loop() {
   printBME280Data(&Serial);
   delay(500);
}

void printBME280Data(Stream* client) {
  float temp(NAN), hum(NAN), pres(NAN);

  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);

  bme.read(pres, temp, hum, tempUnit, presUnit);

  if (tempUnit == BME280::TempUnit_Fahrenheit) {
    temp = toCelsius(temp);
  }

  const auto ok = ambient.set(static_cast<uint8_t>(AmbientField::Temperature), temp)
    && ambient.set(static_cast<uint8_t>(AmbientField::Humidity), hum)
    && ambient.set(static_cast<uint8_t>(AmbientField::AtmosphericPressure), pres);
  if (ok) {
    ambient.send();
  }

  client->print("Temp: ");
  client->print(temp);
  client->print("°C");
  client->print("\t\tHumidity: ");
  client->print(hum);
  client->print("% RH");
  client->print("\t\tPressure: ");
  client->print(pres);
  client->println("Pa");

  delay(1000);
}
