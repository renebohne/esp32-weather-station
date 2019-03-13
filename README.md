# esp32-weather-station

esp32 with DHT22 and GP2Y1010AU0F

board: https://www.amazon.de/Diymore-Wireless-Bluetooth-Batterie-Entwicklung/dp/B077D7PVFC

Dust Sensor: https://www.waveshare.com/wiki/Dust_Sensor

Since the internal ADC of the ESP32 did not work with the sensor, this code uses an ADS1115 connected to SDA 21, SCL 22.

From the dust sensor manual:

It is applied to detect of dust in the air, such as the applications of Air Purifier, Air Monitor and PM2.5 Detector.

* Enable the internal infrared emitting diode by setting the pin ILED to HIGH.

* Wait 0.28ms, then the external controller starts to sample the voltage from the pin AOUT of the module. Notes that the output wave will take 0.28ms to reach steady state after the internal infrared emitting diode is enabled, as Figure 2 shows.

* There is a period of 0.04ms for sampling. When finished, set the pin ILED to LOW to disable the internal infrared emitting diode.

* Calculate the dust concentration according to the relationship between output voltage and dust concentration. For more detailed information, please refer to the relative Demos. Note: The output voltage has been divided (see schematic), so that the voltage measurement should x 11 to get the actual voltage.


# Important 

The onboard voltage divider takes the output of the Sharp dust sensor (range: 0V..4V) and divides it by 11. Thus, the Waveshare board outsputs 0mV..400mV.

The default configuration of the esp32 Arduino core according to https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-adc.h
is:
* analogReadResolution(12)
*  analogSetWidth(12)
* analogSetCycles(8)
* analogSetSamples(1)
* analogSetClockDiv(1)
* analogSetAttenuation(ADC_11db)


The main problems are the high attenuation of 11dB and the non-linear behaviour of the ADC at 12-bit.
So we need to change it to:

* analogSetAttenuation(ADC_0db)
* analogReadResolution(10)

WE DON'T USE THE INTERNAL ADC ANYMORE, but added an external ADS1115
