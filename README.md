# ESP32 environment sensor

This project encompasses hardware and firmware for a WiFi-enabled temperature and humidity sensor.
It has a battery life of ~3 months on a single lithium-ion battery (3000mAh), by reporting temperature and humidity every 15 minutes.

Documentation is a little bit sparse, as this project is for my personal use.

# Recommended hardware changes

After rev1 hardware was built, I realized a few things:
* Either don't use resistor divider to measure battery voltage, or connect it only when awake via transistor (it consumes ~25uA).
* Change MIC5504 voltage regulator to one with less leakage current- it consumes ~40uA.

# Pictures
![PCBs assembled in reflow oven](https://raw.githubusercontent.com/festlv/esp32-temperature-humidity-sensor/master/hw/img/assembled-pcbs.jpg)
![3D printed case with components](https://raw.githubusercontent.com/festlv/esp32-temperature-humidity-sensor/master/hw/img/case-inside.jpg)
![Outside of 3D printed case](https://raw.githubusercontent.com/festlv/esp32-temperature-humidity-sensor/master/hw/img/case-outside.jpg)






