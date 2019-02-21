#include <Wire.h>
#include "SSD1306Wire.h"

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <WiFi.h>
#include <PubSubClient.h>

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




// Update these with values suitable for your network.

const char* ssid = "<YOUR_SSID>";
const char* password = "<YOUR_WIFI_PASSWORD>";
const char* mqtt_server = "<IP_OF_YOUR_MQTT_BROKER>";


//DUST SENSOR stuff
#define        COV_RATIO                       0.2            //ug/mmm / mv
#define        NO_DUST_VOLTAGE                 0            //mv 400
#define        SYS_VOLTAGE                     1000

/*
I/O define
*/
const int iled = 23;                                            //drive the led of sensor
const int vout = 34;                                            //analog input pin

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


//MQTT STUFF

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  display.drawString(10, 50, String("IP: "));
  display.drawString(60, 50, String(WiFi.localIP()));

}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    //digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on

  } else {
    //digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
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
  //display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  Serial.println(F("esp32 Weather Station"));


  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(10, 10, String("esp32 Weather Station"));
  display.drawString(10, 20, String("connecting wifi..."));
  display.display();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

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
  if (!client.connected()) {
  reconnect();
}
client.loop();



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
  }

  display.drawString(10, 30, String("Dust: "));
  display.drawString(60, 30, String(density));

  display.drawString(10, 40, String("Voltage: "));
  display.drawString(60, 40, String(voltage));

  //display.drawString(10, 50, String("Hall: "));
  //display.drawString(60, 50, String(hallRead()));

  display.display();

  //publish to mqtt
  sdensity.toCharArray(msg, 50);
  Serial.print("Publish message: ");
  Serial.println(msg);

  client.publish("dust", msg);



}
