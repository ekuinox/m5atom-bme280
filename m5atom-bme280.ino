#include <BME280I2C.h>
#include <Wire.h>
#include <M5Atom.h>

#define SERIAL_BAUD 115200
#define SCK_PIN (19) // SCL
#define SDI_PIN (22) // SDA

BME280I2C bme;

void setup() {
  M5.begin(true, false, true);
  delay(50);

  // 赤色に点灯させる
  M5.dis.drawpix(0, 0x00ff00);
  
  Serial.begin(SERIAL_BAUD);

  while (!Serial);

  pinMode(SCK_PIN, INPUT_PULLUP);
  pinMode(SDI_PIN, INPUT_PULLUP);

  Wire.begin(SDI_PIN, SCK_PIN);

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

   client->print("Temp: ");
   client->print(temp);
   client->print("°"+ String(tempUnit == BME280::TempUnit_Celsius ? 'C' :'F'));
   client->print("\t\tHumidity: ");
   client->print(hum);
   client->print("% RH");
   client->print("\t\tPressure: ");
   client->print(pres);
   client->println("Pa");

   delay(1000);
}
