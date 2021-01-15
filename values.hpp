class Ambient;
class Stream;
class BME280I2C;
class InfluxDBClient;

// 閾値
template <typename T>
struct Threshold {
  /**
   * 最小値
   */
  const T min;

  /**
   * 最大値
   */
  const T max;
  
  /**
   * valueが有効か
   */
  auto isValid(const T & value) const -> bool {
    return !isnan(value) && this->min <= value && value <= this->max;
  }
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
   * 気圧 (hPa)
   */
  const float atmosphericPressure;

  /**
   * 有効か
   */
  const bool isValid;

  /**
   * Ambientにセンサの値を送信する
   */
  auto sendToAmbient(Ambient & ambient) const -> bool;

  /**
   * influxに値を送信する
   */
  auto sendToInflux(InfluxDBClient & client, const char * const measurement) const -> bool;

  /**
   * Serialに出力する
   */
  auto println(Stream & serial) const -> void;

  /**
   * センサの値を取得する
   */
  static auto read(BME280I2C & bme) -> Values;
};
