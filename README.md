# esp32-Web-Server-and-Sd-card
Arduino project for a Freenove Wrover esp32-S3

The device is a Freenove Wrover esp-s3 with a camera, a PIR and LED strip attached. The LED strip powered through a separate usb and switched on and off by a relay. When the PIR goes high, the LED strip is turned on through the relay and then 3 photos are taken and stored to the SD card. The file name is the time and date the photo was taken. The web site will display all the photos on the root of the SD card. The web has functions to delete or download individual photos and a function to delete all the photos at once.

Going through the code
The code is an amalgamation of esp32 examples and code from others on the internet.

The includes are standard for a web server that connects to an NTP server, has a camera and an SD card.
Things that may/wil change for your device.
If you have a different board, the camera and pins will change
eg, the wrover camera pins are different to the freenove wroom camera and pins
the wrover defines are

#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
#define SD_MMC_CMD 15 //Please do not modify it.
#define SD_MMC_CLK 14 //Please do not modify it. 
#define SD_MMC_D0  2  //Please do not modify it.

The Wroom defines are
#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM
#define SD_MMC_CMD 38 //Please do not modify it.
#define SD_MMC_CLK 39 //Please do not modify it. 
#define SD_MMC_D0  40  //Please do not modify it.

The SD code is taken from the SD esp32 example but you need to use the  SD_MMC.h library
#include "SD_MMC.h"
not the normal SD library

The two pins used are 33 and 32
#define FLASH_LED 32
#define PIR 33

There are not many pins free after you add the camera and SD card.

Functions

loadFromSdCard
This has been taken from the esp32 SD web server example. The SD library has been changed to the SD_MCC library

handleNotFound
This has been taken from the esp32 SD web server example.

HomePage
This creates the home page with all the pictures on the root of the SD card. The approach has been taken from the internet (I will add the attribute when I find the page again)
listDironWebpage
This creates the list of files on the SD card.

File_Download()
SD_file_download(String filename)
This has been taken from the esp32 SD web server example.

File_Delete_All()
This has been taken modified from the esp32 SD web server example.

File_Delete()
SD_file_delete(String filename)
This has been taken from the esp32 SD web server example.

SnapShot()
This code is an amalgamation of the ESP32 camera example, the SD web server example and the NTP time example.
It sets up the file name using the current time. It takes care of the time format so the file name is consistent. This makes sorting on the web page easy.
It opens the file for writing, takes the picture and loads the picture buffer into the file and uploads it to the SD card.






