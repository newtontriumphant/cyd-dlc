# cyd-dlc
Hej! Digital Live Clock (DLC) is a simple program written entirely in C++ that can be run on a ESP32-2432S028, better known as the CYD (Cheap Yellow Display)!

It's also technically downloadable content, soooo a DLC is a DLC!

The DLC has two modes: in clock mode, it tracks temperature, humidity (via a DHT22 sensor connected to GPIO), and real-time clock data synced with NTP over the ESP32's WiFi Chip.

In timer mode, it also doubles as a Pomodoro timer! You can set a certain duration, ranging from 1 to 60 minutes, and after you complete the timer, you're rewarded with a 10% break!

In the future, I plan to add some cool GIFs and animations as well! :3

WIRING:

ESP32-2432S028 x1

3v3  GND  GPIO22  GPIO27
 |    |     |        X
 |    |     |
VIN  GND  SIGNAL

DHT22 BREAKOUT BOARD x1