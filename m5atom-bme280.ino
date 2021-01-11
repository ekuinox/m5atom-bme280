#include <BME280I2C.h>
#include <Wire.h>
#include <M5Atom.h>
#include <WiFi.h>
#include <Ambient.h>
#include "./conf.h"

constexpr auto SERIAL_BAUD = 115200;
constexpr auto SCK_PIN = 19; // SCL
constexpr auto SDI_PIN = 22; // SDA
constexpr auto AMBIENT_SEND_INTERVAL = 60 * 1000; // Ambient

enum class AmbientField {
  Temperature = 1,
  Humidity = 2,
  AtmosphericPressure = 3,
};

struct Values {
  /**
   * 摂氏温度
   */
  float temperature;

  /**
   * 湿度
   */
  float humidity;

  /**
   * 気圧 (Pa)
   */
  float atmosphericPressure;

  /**
   * Ambientにセンサの値を送信する
   */
  auto sendToAmbient(Ambient & ambient) const -> bool;

  /**
   * Serialに出力する
   */
  auto println(Stream & serial) const -> void;

  /**
   * センサの値を取得する
   */
  static auto read(BME280I2C & bme) -> Values;
};

class Time {
  /**
   * 最終更新時刻
   */
  uint64_t lastUpdated = 0;

public:
  /**
   * lastUpdatedを更新する
   */
  auto update() -> void;

  /**
   * 指定した時間が過ぎているか
   */
  auto isElapsed(const uint64_t & span) const -> bool;
};

WiFiClient wifi;
Ambient ambient;
BME280I2C bme;

auto setup() -> void {
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

auto loop() -> void {
  static auto time = Time();
  const auto values = Values::read(bme);
  values.println(Serial);
  if (time.isElapsed(AMBIENT_SEND_INTERVAL) && values.sendToAmbient(ambient)) {
    Serial.println("send values to ambient");
    time.update();
  }
  delay(1000);
}

auto Values::read(BME280I2C & bme) -> Values {
  const auto tempUnit = BME280::TempUnit(BME280::TempUnit_Celsius);
  const auto presUnit = BME280::PresUnit(BME280::PresUnit_Pa);
  auto temp = float { NAN };
  auto hum = float { NAN };
  auto pres = float { NAN };

  bme.read(pres, temp, hum, tempUnit, presUnit);

  return {
    temp, hum, pres,
  };
}

auto Values::sendToAmbient(Ambient & ambient) const -> bool {
  const auto ok = ambient.set(static_cast<uint8_t>(AmbientField::Temperature), this->temperature)
    && ambient.set(static_cast<uint8_t>(AmbientField::Humidity), this->humidity)
    && ambient.set(static_cast<uint8_t>(AmbientField::AtmosphericPressure), this->atmosphericPressure);
  return ok && ambient.send();
}

auto Values::println(Stream & serial) const -> void {
  serial.print("Temp: ");
  serial.print(this->temperature);
  serial.print("°C");
  serial.print("\t\tHumidity: ");
  serial.print(this->humidity);
  serial.print("% RH");
  serial.print("\t\tPressure: ");
  serial.print(this->atmosphericPressure);
  serial.println("Pa");
}

auto Time::update() -> void {
  this->lastUpdated = millis();
}

auto Time::isElapsed(const uint64_t & span) const -> bool {
  // 0は初回とみなしてtrueを返す
  if (this->lastUpdated == 0) {
    return true;
  }
  const auto current = millis();
  if (current >= this->lastUpdated) {
    return current - this->lastUpdated > span;
  }
  // 過去にupdateした時点よりcurrentの方が小さい場合
  return -1UL - this->lastUpdated + current > span; // 桁溢れまでの量 + 桁溢れした量 > span
}
