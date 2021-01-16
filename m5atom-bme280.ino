#include <BME280I2C.h>
#include <Wire.h>
#include <M5Atom.h>
#include <WiFi.h>
#include <Ambient.h>
#include <Ethernet.h>
#include <InfluxDbClient.h>
#include "./values.hpp"
#include "./time.hpp"
#include "./conf.hpp"

#define ENABLE_SERVER (0)

constexpr auto SERIAL_BAUD = 115200;
constexpr auto SCK_PIN = 19; // SCL
constexpr auto SDI_PIN = 22; // SDA
constexpr auto AMBIENT_SEND_INTERVAL = 60 * 1000; // Ambient

// LED用の色
enum class ColorGRB : uint32_t {
  Red = 0x00ff00,
  Green = 0xff0000,
  Orange = 0xa5ff00,
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

/**
 * LEDを点灯させる
 */
auto lit(const ColorGRB & color) -> void;

auto wifi = WiFiClient();
auto ambient = Ambient();
auto bme = BME280I2C();
auto influx = InfluxDBClient(INFLUX_URL, INFLUX_DB);

#if ENABLE_SERVER
WiFiServer server(80);
#endif

auto setup() -> void {
  // Start M5
  M5.begin(true, false, true);
  delay(50);
  // 赤色に点灯させる
  lit(ColorGRB::Red);

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

  // Start influxdb
  influx.setConnectionParamsV1(INFLUX_URL, INFLUX_DB, INFLUX_USERNAME, INFLUX_PASSWORD);
  if (!influx.validateConnection()) {
    Serial.println("Could not connect influxdb");
  }

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

  #if ENABLE_SERVER
  // Start server
  server.begin();
  #endif

  // chipModelがBME280か
  const auto isBME280 = bme.chipModel() == BME280::ChipModel::ChipModel_BME280;
  lit(isBME280 ? ColorGRB::Green : ColorGRB::Red); //GRB

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
    // 値が期待する範囲内か
    if (values.isValid) {
      values.println(Serial);
      const auto ok = time.isElapsed(AMBIENT_SEND_INTERVAL)
        && values.sendToInflux(influx, INFLUX_MEASUREMENT)
        && values.sendToAmbient(ambient);
      if (ok) {
        Serial.println("send values to ambient & influx");
        time.update();
      }
      lit(ColorGRB::Green); // 緑色
    } else {
      lit(ColorGRB::Orange); // オレンジ色
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

auto lit(const ColorGRB & color) -> void {
  M5.dis.drawpix(0, static_cast<uint32_t>(color));
}
