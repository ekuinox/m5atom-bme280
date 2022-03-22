# m5atom-bme280

M5AtomでBME280の値を取得して、 Ambient, Influx にアップロードするなどする。

## 導入

- ボードマネージャから *esp32* を追加
  * `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` を*追加のボードマネージャのURL*として設定しておく
  * ボードは **M5Stack-ATOM** を選択
- ライブラリを導入
  * *BME280* ... [finitespace/BME280](https://github.com/finitespace/BME280)
  * *Ambient ESP32 ESP8266 lib* ... [AmbientDataInc/Ambient_ESP8266_lib](https://github.com/AmbientDataInc/Ambient_ESP8266_lib)
  * *ESP8266 Influxdb* ... [AmbientDataInc/Ambient_ESP8266_lib](https://github.com/AmbientDataInc/Ambient_ESP8266_lib)
- `conf.hpp.sample` を `conf.hpp` にコピーして修正
