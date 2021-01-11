#include <BME280I2C.h>
#include <Wire.h>
#include <M5Atom.h>
#include <WiFi.h>
#include <Ambient.h>
#include <Ethernet.h>
#include "./conf.h"

#define ENABLE_SERVER (0)

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
  const float temperature;

  /**
   * 湿度
   */
  const float humidity;

  /**
   * 気圧 (Pa)
   */
  const float atmosphericPressure;

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

/**
 * 値を読み取りアップロードなどするタスク
 */
auto readValuesTask(void * arg) -> void;

#if ENABLE_SERVER
/**
 * Httpリクエストを読み取り応答するタスク
 */
auto readHttpTask(void * arg) -> void;
#endif

/**
 * 状態をシリアルに出力する
 */
auto dumpStatus(Stream & stream) -> void;

WiFiClient wifi;
WiFiServer server(80);
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

  // Start server
  server.begin();
  
  // chipModelがBME280あれば緑、そうでなければ赤に点灯させる
  M5.dis.drawpix(0, bme.chipModel() == BME280::ChipModel::ChipModel_BME280 ? 0xff0000 : 0x00ff00); //GRB

  xTaskCreatePinnedToCore(readValuesTask, "readValuesTask", 4096, NULL, 1, NULL, 0);
  #if ENABLE_SERVER
  xTaskCreatePinnedToCore(readHttpTask, "readHttpTask", 4096, NULL, 1, NULL, 1);
  #endif
}

auto loop() -> void {
  // ボタンが押された際に状態を出力する
  if (M5.Btn.wasPressed()) {
    dumpStatus(Serial);
  }
  M5.update();
  delay(500);
  //
}

auto readValuesTask(void * arg) -> void {
  auto time = Time();
  while (true) {
    const auto values = Values::read(bme);
    values.println(Serial);
    if (time.isElapsed(AMBIENT_SEND_INTERVAL) && values.sendToAmbient(ambient)) {
      Serial.println("send values to ambient");
      time.update();
    }
    delay(1000);
  }
}

#if ENABLE_SERVER
auto readHttpTask(void * arg) -> void {
  while (true) {
    auto client = server.available();
    if (!client) continue;
    while (client.connected()) {
      if (!client.available()) continue;
      const auto values = Values::read(bme);
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println();
      client.print("[");
      client.print(values.temperature);
      client.print(",");
      client.print(values.humidity);
      client.print(",");
      client.print(values.atmosphericPressure);
      client.println("]");
      break;
    }
    while (client.connected());
    client.stop();
  }
}
#endif

auto dumpStatus(Stream & stream) -> void {
  // IPアドレスを吐き出す
  Serial.print("IP Address => ");
  Serial.println(WiFi.localIP());
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
