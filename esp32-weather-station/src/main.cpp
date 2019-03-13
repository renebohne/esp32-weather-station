#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include <Wire.h>
#include "SSD1306Wire.h"

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <Adafruit_ADS1015.h>

#define DHTPIN 18
#define DHTTYPE    DHT22     // DHT 22 (AM2302)

#define USE_MQTT 1

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

//The BOOT button is connected to gpio 0.
#define BUTTON_PIN 0

//blue onboard LED
#define LED_PIN 16

#define OLED_ADDRESS 0x3C
#define OLED_SDA 21 //5
#define OLED_SCL 22 //4

SSD1306Wire display(OLED_ADDRESS, OLED_SDA, OLED_SCL);


Adafruit_ADS1115 ads1115(0x48);


const char* ssid = "<YOUR_SSID>";
const char* password = "<YOUR_WIFI_PASSWORD>";

const char* mqtt_server = "mqtt.tingg.io";

const char* thing_id = "<YOUR-TINGG-THINGID>";
const char*  key = "<YOUR-TINGG-THINGKEY>";
const char* username = "thing";


//DUST SENSOR stuff
#define        COV_RATIO                       0.2            //ug/mmm / mv
#define        NO_DUST_VOLTAGE                 0            //mv 400
#define        SYS_VOLTAGE                     1000

const int iled = 23;                                            //drive the led of sensor
const int vout = 34;                                            //analog input pin


float density, voltage;
int   adcvalue;

// Pins Config
const int LightPin = 16;
const int ButtonPin = 0;

WiFiClient espClient;
PubSubClient client(espClient);


// Vars
int val;
char buf[12];
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {

  delay(10);

  if(USE_MQTT==1)
  {
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());

      display.drawString(10, 50, String("IP: "));
      display.drawString(60, 50, String(WiFi.localIP()));
  }
  randomSeed(micros());

}


void reconnect() {
  if(USE_MQTT!=1)
  {
    return;
  }
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(thing_id, username, key)) {
      Serial.println("connected");
      //client.subscribe(subTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


String message(byte* payload, unsigned int length) {
  payload[length] = '\0';
  String s = String((char*)payload);
  return s;
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println(topic);
  String msg = message(payload, length);

  if (msg == "ON") {
    digitalWrite(LightPin,LOW);
  }
  else {
    digitalWrite(LightPin,HIGH);
  }
}



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
  digitalWrite(iled, LOW);

  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialising the UI will init the display too.
  display.init();
  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  Serial.println(F("esp32 Weather Station"));

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(10, 10, String("esp32 Weather Station"));


  ads1115.setGain(GAIN_FOUR);    // 4x gain   +/- 1.024V  1 bit = 0.5mV
  //ads1115.begin();

  if(USE_MQTT==1)
  {
    display.drawString(10, 20, String("connecting wifi..."));
    display.display();
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
  }

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
  digitalWrite(iled, HIGH);
  delayMicroseconds(280);

  //get adcvalue
  int16_t adc0;//1 bit is 0.5mV
  adc0 = ads1115.readADC_SingleEnded(0);
  digitalWrite(iled, LOW);

  Serial.print("adc0: ");
  Serial.println(adc0);


  //adcvalue = Filter(adcvalue);

  //convert adc value to voltage [mV]
  voltage = adc0 *0.5;

  Serial.print("voltage: ");
  Serial.println(voltage,DEC);

  //voltage to density
  if(voltage >= NO_DUST_VOLTAGE)
  {
    voltage -= NO_DUST_VOLTAGE;

    density = voltage * COV_RATIO;
  }
  else
  {
    density = 0;
  }

  //display the result
  Serial.print("The current dust concentration is: ");
  Serial.print(density);
  Serial.print(" ug/m3\n");
}

void loop() {
  if(USE_MQTT)
  {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
  }


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

  String str_density = String(density);

  display.clear();
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    String str_temperature = String(event.temperature);
    Serial.print(F("Temperature: "));
    Serial.print(str_temperature);
    Serial.println(F("°C"));

    display.drawString(10, 10, String("Temp: "));
    display.drawString(60, 10, str_temperature);

    if(USE_MQTT==1)
    {
      //publish to mqtt
      str_temperature.toCharArray(msg, 50);
      Serial.print("Publish temperature message: ");
      Serial.println(msg);
      client.publish("temperature", msg);
    }

  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    String str_humidity = String(event.relative_humidity);
    Serial.print(F("Humidity: "));
    Serial.print(str_humidity);
    Serial.println(F("%"));

    display.drawString(10, 20, String("Humidity: "));
    display.drawString(60, 20, str_humidity);
    //publish to mqtt
    if(USE_MQTT==1)
    {
      str_humidity.toCharArray(msg, 50);
      Serial.print("Publish humidity message: ");
      Serial.println(msg);
      client.publish("humidity", msg);
    }
  }

  display.drawString(10, 30, String("Dust: "));
  display.drawString(60, 30, String(density));

  display.drawString(10, 40, String("Voltage[mV]: "));
  display.drawString(60, 40, String(voltage));

  display.display();

  if(USE_MQTT==1)
  {
    //publish to mqtt
    str_density.toCharArray(msg, 50);
    Serial.print("Publish dust density message: ");
    Serial.println(msg);
    client.publish("density", msg);
  }
}
