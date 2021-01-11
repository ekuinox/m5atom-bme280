#include <Ambient.h>
#include <BME280I2C.h>
#include "./values.hpp"

// Ambientのチャートフィールド対応
enum class AmbientField {
  Temperature = 1,
  Humidity = 2,
  AtmosphericPressure = 3,
};

constexpr auto TEMPERATURE = Threshold<float> { -40.0f, 85.0f };
constexpr auto HUMIDITY = Threshold<float> { 0.0f, 100.0f };
constexpr auto ATMOSPHERIC_PRESSURE = Threshold<float> { 300.0f, 1100.0f };
constexpr auto TEMPERATURE_UNIT = BME280::TempUnit_Celsius;
constexpr auto ATMOSPHERIC_PRESSURE_UNIT = BME280::PresUnit_hPa;

auto Values::read(BME280I2C & bme) -> Values {
  auto temp = float { NAN };
  auto hum = float { NAN };
  auto pres = float { NAN };

  bme.read(pres, temp, hum, TEMPERATURE_UNIT, ATMOSPHERIC_PRESSURE_UNIT);

  const auto isValid = TEMPERATURE.isValid(temp) && HUMIDITY.isValid(hum) && ATMOSPHERIC_PRESSURE.isValid(pres);

  return {
    temp, hum, pres, isValid
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
  serial.println("hPa");
}
