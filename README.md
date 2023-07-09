# CPU
 4bitCPU folder, containing subfolders with board specific Arduino projects.

Release v1.0 - Full Functionality

 Currently implemented:
 - CPU_ALU
 - CPU_MANUAL
 - CPU_CONTROL (not fully functional yet, just demo)
 - Global reset
 - Demo modes

 Requires the following libraries:

 https://github.com/paladin32776/LED_CPU
 https://github.com/paladin32776/PCA9955
 https://github.com/paladin32776/NoBounceButtons
 https://github.com/paladin32776/EnoughTimePassed
 https://github.com/sandeepmistry/arduino-CAN
 https://github.com/paladin32776/DISP_CPU
 https://github.com/Bodmer/TFT_eSPI


If using PCA9955B instead of PCA9955, please get:

https://github.com/paladin32776/PCA9955B

and modify the LED_CPU library accordingly (replace PCA9955 with PCA9955B).

Arduino compile settings:

Board: ESP32 Dev Module
CPU Frequency: 240MHz
Flash Frequency: 80 MHz
Flash Mode: QIO
Flash Size: 4MB (32Mb)
Partition Scheme: Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)
PSRAM: Disabled
