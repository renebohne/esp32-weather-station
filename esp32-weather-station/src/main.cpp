#include <Wire.h>
#include "SSD1306Wire.h"

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 18
#define DHTTYPE    DHT22     // DHT 22 (AM2302)

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

//The BOOT button is connected to gpio 0.
#define BUTTON_PIN 0

#define OLED_ADDRESS 0x3C
#define OLED_SDA 5
#define OLED_SCL 4

SSD1306Wire display(OLED_ADDRESS, OLED_SDA, OLED_SCL);


//DUST SENSOR stuff
#define        COV_RATIO                       0.2            //ug/mmm / mv
#define        NO_DUST_VOLTAGE                 0            //mv 400
#define        SYS_VOLTAGE                     1000

/*
I/O define
*/
const int iled = 23;                                            //drive the led of sensor
const int vout = 36;                                            //analog input on pin VP (gpio 36)

/*
variable
*/
float density, voltage;
int   adcvalue;

/*
private function
*/
int Filter(int m)
{
  static int flag_first = 0, _buff[10], sum;
  const int _buff_max = 10;
  int i;

  if(flag_first == 0)
  {
    flag_first = 1;

    for(i = 0, sum = 0; i < _buff_max; i++)
    {
      _buff[i] = m;
      sum += _buff[i];
    }
    return m;
  }
  else
  {
    sum -= _buff[0];
    for(i = 0; i < (_buff_max - 1); i++)
    {
      _buff[i] = _buff[i + 1];
    }
    _buff[9] = m;
    sum += _buff[9];

    i = sum / 10.0;
    return i;
  }
}



void setup() {
  Serial.begin(115200);
  // Initialize device.
  dht.begin();

  pinMode(iled, OUTPUT);
  digitalWrite(iled, LOW);                                     //iled default closed



	analogReadResolution(10);
	analogSetAttenuation(ADC_0db);



  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  Serial.println(F("esp32 Weather Station"));

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(10, 10, String("esp32 Weather Station"));
  display.display();

  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
}

void readDustSensor(void)
{
  /*
  get adcvalue
  */
  digitalWrite(iled, HIGH);
  delayMicroseconds(280);

  adcvalue = analogRead(vout);
  digitalWrite(iled, LOW);

  //adcvalue = Filter(adcvalue);

  /*
  covert voltage (mv)
  */
  //voltage = (SYS_VOLTAGE / 1024.0) * adcvalue * 11;
  voltage = (SYS_VOLTAGE / 1024.0) * adcvalue *11;


  /*
  voltage to density
  */
  if(voltage >= NO_DUST_VOLTAGE)
  {
    voltage -= NO_DUST_VOLTAGE;

    density = voltage * COV_RATIO;
  }
  else
    density = 0;

  /*
  display the result
  */
  Serial.print("The current dust concentration is: ");
  Serial.print(density);
  Serial.print(" ug/m3\n");


}

void loop() {
  // Delay between measurements.
  if(delayMS <1000)
  {
    delay(1000);
  }
  else
  {
    delay(delayMS);
  }

  readDustSensor();

  display.clear();
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));

    display.drawString(10, 10, String("Temp: "));
    display.drawString(60, 10, String(event.temperature));

  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));

    display.drawString(10, 20, String("Humidity: "));
    display.drawString(60, 20, String(event.relative_humidity));
  }

  display.drawString(10, 30, String("Dust: "));
  display.drawString(60, 30, String(density));

  display.drawString(10, 40, String("Voltage: "));
  display.drawString(60, 40, String(voltage));

  display.display();
}
