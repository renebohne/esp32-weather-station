# esp32-weather-station

esp32 with DHT22 and GP2Y1010AU0F

board: https://www.amazon.de/Diymore-Wireless-Bluetooth-Batterie-Entwicklung/dp/B077D7PVFC


Dust Sensor: https://www.waveshare.com/wiki/Dust_Sensor

From the manual:

It is applied to detect of dust in the air, such as the applications of Air Purifier, Air Monitor and PM2.5 Detector.

* Enable the internal infrared emitting diode by setting the pin ILED to HIGH.

* Wait 0.28ms, then the external controller starts to sample the voltage from the pin AOUT of the module. Notes that the output wave will take 0.28ms to reach steady state after the internal infrared emitting diode is enabled, as Figure 2 shows.

* There is a period of 0.04ms for sampling. When finished, set the pin ILED to LOW to disable the internal infrared emitting diode.

* Calculate the dust concentration according to the relationship between output voltage and dust concentration. For more detailed information, please refer to the relative Demos. Note: The output voltage has been divided (see schematic), so that the voltage measurement should x 11 to get the actual voltage.
