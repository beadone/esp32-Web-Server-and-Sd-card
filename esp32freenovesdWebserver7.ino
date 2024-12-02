/**********************************************************************
  Filename    : SDMMC Test
  Description : The SD card is accessed using the SDMMC one-bit bus
  Auther      : www.freenove.com
  Modification: 2024/06/20
**********************************************************************/
#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPI.h>
#include <time.h>

#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "sdkconfig.h"
#include "camera_index.h"
#include "camera_pins.h"

String webpage = "";
#define ServerVersion "1.0"
#include "CSS.h"
#include "sd_read_write.h"
#include "SD_MMC.h"

#define SD_MMC_CMD 15 //Please do not modify it.
#define SD_MMC_CLK 14 //Please do not modify it. 
#define SD_MMC_D0  2  //Please do not modify it.
#define DBG_OUTPUT_PORT Serial
int rowcounter = 0;

#define FLASH_LED 32
#define PIR 33

bool   SD_present = false;


const char *ssid = "changethis";
const char *password = "change this";

#define   servername "esp32sd"  //you can change this
WebServer server(80);

//const long utcOffsetInSeconds = 36000;
//const long utcOffsetInSeconds = 39600;

String formattedDate;

long timezone = 11;
byte daysavetime = 1;
struct tm tmstruct;

bool loadFromSdCard(String path) {
  String dataType = "text/plain";
  if (path.endsWith("/")) {
    path += "index.htm";
  }

  if (path.endsWith(".src")) {
    path = path.substring(0, path.lastIndexOf("."));
  } else if (path.endsWith(".htm")) {
    dataType = "text/html";
  } else if (path.endsWith(".css")) {
    dataType = "text/css";
  } else if (path.endsWith(".js")) {
    dataType = "application/javascript";
  } else if (path.endsWith(".png")) {
    dataType = "image/png";
  } else if (path.endsWith(".gif")) {
    dataType = "image/gif";
  } else if (path.endsWith(".jpg")) {
    dataType = "image/jpeg";
  } else if (path.endsWith(".ico")) {
    dataType = "image/x-icon";
  } else if (path.endsWith(".xml")) {
    dataType = "text/xml";
  } else if (path.endsWith(".pdf")) {
    dataType = "application/pdf";
  } else if (path.endsWith(".zip")) {
    dataType = "application/zip";
  }

  File dataFile = SD_MMC.open(path.c_str());
  if (dataFile.isDirectory()) {
    path += "/index.htm";
    dataType = "text/html";
    dataFile = SD_MMC.open(path.c_str());
  }

  if (!dataFile) {
    return false;
  }

  if (server.hasArg("download")) {
    dataType = "application/octet-stream";
  }

  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    DBG_OUTPUT_PORT.println("Sent less data than expected!");
  }

  dataFile.close();
  return true;
}

void handleNotFound() {
  if (SD_present && loadFromSdCard(server.uri())) {
    return;
  }
  String message = "SDCARD Not Detected or file type not supported\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  DBG_OUTPUT_PORT.print(message);
}

void setup(){
    Serial.begin(115200);
  DBG_OUTPUT_PORT.setDebugOutput(true);
  DBG_OUTPUT_PORT.print("\n");
  //WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  DBG_OUTPUT_PORT.print("Connecting to ");
  DBG_OUTPUT_PORT.println(ssid);
  delay(2000);

  // Wait for connection
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 10) {  //wait 5 seconds
    delay(500);
    DBG_OUTPUT_PORT.print(".");
  }
  if (i == 11) {
    DBG_OUTPUT_PORT.print("Could not connect to");
    DBG_OUTPUT_PORT.println(ssid);
    ESP.restart();
    while (1) {
      delay(500);
    }
  }
  
  DBG_OUTPUT_PORT.print("Connected! IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());


  Serial.println("Contacting Time Server");
  configTime(3600 * timezone, daysavetime * 3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
  
  //delay(2000);
  tmstruct.tm_year = 0;
  getLocalTime(&tmstruct, 5000);
  Serial.printf(
    "\nNow is : %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min,
    tmstruct.tm_sec
  );
  Serial.println("");
  
  if (MDNS.begin(servername)) {
    MDNS.addService("http", "tcp", 80);
    DBG_OUTPUT_PORT.println("MDNS responder started");
    DBG_OUTPUT_PORT.print("You can now connect to http://");
    DBG_OUTPUT_PORT.print(servername);
    DBG_OUTPUT_PORT.println(".local");
  }

    
    SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
    if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
      Serial.println("Card Mount Failed");
      return;
    }
    uint8_t cardType = SD_MMC.cardType();
    if(cardType == CARD_NONE){
        Serial.println("No SD_MMC card attached");
        return;
    } else{
      SD_present = true;
    }

    Serial.print("SD_MMC Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

  // Note: Using the ESP32 and SD_Card readers requires a 1K to 4K7 pull-up to 3v3 on the MISO line, otherwise they do-not function.
  //----------------------------------------------------------------------   
  ///////////////////////////// Server Commands 
  server.on("/",         HomePage);
  server.on("/download", File_Download);
  server.on("/upload",   File_Upload);
  server.on("/fupload",  HTTP_POST,[](){ server.send(200);}, handleFileUpload);
  server.on("/capture", SnapShot);
  server.on("/delete", File_Delete);
  server.on("/deleteall", File_Delete_All);
  server.onNotFound(handleNotFound);
  ///////////////////////////// End of Request commands
  server.begin();
  Serial.println("HTTP server started");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }else {
    Serial.println("camera initalised");
  }

  //listDir(SD_MMC, "/", 0);


  pinMode(FLASH_LED, OUTPUT);
  pinMode(PIR, INPUT);
  digitalWrite(PIR,LOW);
  digitalWrite(FLASH_LED,HIGH);
}



void loop(void){
  server.handleClient(); // Listen for client connections
  if(digitalRead(PIR)) {
    Serial.print("Movement detected.");
    digitalWrite(FLASH_LED,LOW);
    delay(500);
    SnapShot();
    delay(500);
    SnapShot();
    delay(500);
    SnapShot(); 
    delay(1000);   
    digitalWrite(FLASH_LED,HIGH);
    Serial.print("flash off");


  }

  //
}
// All supporting functions from here...
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void HomePage(){
  SendHTML_Header();
  webpage += F("<table><tr><th>Picture</th><th>Name</th><th>Download</th><th>Delete</th></tr>");
  listDironWebpage(SD_MMC, "/");
  webpage += F("</table><hr>");
  //webpage += F("");

  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void listDironWebpage(fs::FS &fs, const char * dirname){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }
    
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());

        } else {
            rowcounter = rowcounter +1;
            String rowcounterstr = String(rowcounter);
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());

            webpage += F("<tr><td><img src='");
            webpage += file.name();
            webpage += F("' width='100' height='100'></td><td id=");
            webpage += rowcounterstr;
            webpage += F("/>");
            webpage += file.name();
            //webpage += F("id=");
            //webpage += rowcounterstr;
            webpage += F("</td><td><a href='/download?download=");
            webpage += file.name();
            webpage += F("'><button>Download</button></a></td><td><a href='/delete?delete=/");
            webpage += file.name();
            webpage += F("'><button>Delete</button></a></td></tr>");
  //webpage += F("");

        }
        file = root.openNextFile();
    } 
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Download(){ // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
 
  if (server.args() > 0 ) { // Arguments were received
    if (server.hasArg("download")) SD_file_download(server.arg(0));
  }
  else SelectInput("Enter filename to download","download","download");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_download(String filename){
  fs::FS &fs = SD_MMC;
  if (SD_present) { 
    File download = fs.open("/"+filename);
    if (download) {
      server.sendHeader("Content-Type", "text/text");
      server.sendHeader("Content-Disposition", "attachment; filename="+filename);
      server.sendHeader("Connection", "close");
      server.streamFile(download, "application/octet-stream");
      download.close();
    } else ReportFileNotPresent("download"); 
  } else ReportSDNotPresent();
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Delete_All() {
  fs::FS &fs = SD_MMC;
  File root = fs.open("/");
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }
    
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());

        } else {

            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
            String filename = "/" + String(file.name());
            if(fs.remove(filename)){
                Serial.println("File deleted");
            }

        }
        file = root.openNextFile();
    }
          webpage = "";
          append_page_header();
          webpage += F("<h3>files were successfully deleted</h3>"); 
          webpage += F("<<br>");
          append_page_footer(); 
          server.send(200, "text/html",webpage);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//void deleteFile(fs::FS &fs, const char * path){



void File_Delete(){ // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
  //Serial.println(server.arg(0));
  if (server.args() > 0 ) { // Arguments were received
    //Serial.println(server.arg(0));
    if (server.hasArg("delete")) SD_file_delete(server.arg(0));
  }
  else Serial.println("  wrong delete parameter");
  //else SelectInput("Enter filename to download","download","download");
  
}
void SD_file_delete(String filename){
  fs::FS &fs = SD_MMC;
  if (SD_present) { 
    Serial.printf("Deleting file: %s\n", filename);
    if(fs.remove(filename)){
        Serial.println("File deleted");

                  webpage = "";
          append_page_header();
          webpage += F("<h3>file was successfully deleted</h3>"); 
          webpage += F("<h2> File Name: "); webpage += filename +"</h2>";
          webpage += F("<<br>");
          append_page_footer();

    } else {
        Serial.println("Delete failed");
        
          webpage = "";
          append_page_header();
          webpage += F("<h3>file was NOT deleted</h3>"); 
          webpage += F("<h2> File Name: "); webpage += filename +"</h2>";
          webpage += F("<<br>");
          append_page_footer();
    }
    server.send(200, "text/html",webpage);    
  } 
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SnapShot() {
  camera_fb_t *fb = NULL;
  fs::FS &fs = SD_MMC;
  char str[256] = "";
  //String filename = "/photo_01a.jpg";
  String MM = "";
  String dd = "";
  String hh = "";
  String mm = "";
  String ss = "";

  getLocalTime(&tmstruct, 5000);
  Serial.printf(
    "\nNow is : %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min,
    tmstruct.tm_sec
  );
  Serial.println("");
  MM = String(tmstruct.tm_mon + 1);
  dd = String(tmstruct.tm_mday);
  hh = String(tmstruct.tm_hour);
  mm = String(tmstruct.tm_min);
  ss = String(tmstruct.tm_sec);

  if (MM.length() < 1) MM = "00";
  if (dd.length() < 1) dd = "00";
  if (hh.length() < 1) hh = "00";
  if (mm.length() < 1) mm = "00";
  if (ss.length() < 1) ss = "00";
  
  if (MM.length() < 2) MM = "0" + MM;
  if (dd.length() < 2) dd = "0" + dd;
  if (hh.length() < 2) hh = "0" + hh;
  if (mm.length() < 2) mm = "0" + mm;
  if (ss.length() < 2) ss = "0" + ss;  
  
  formattedDate = String(tmstruct.tm_year + 1900) + MM + dd + hh + mm + ss;

  
  Serial.print("Time: ");
  Serial.println(formattedDate);
  String filename = "/p_" + formattedDate + ".jpg"; 
  
  Serial.print("Saving to SD_MMC: ");
  Serial.println(filename);
  //SD_MMC.remove(filename);                         // Remove a previous version, otherwise data is appended the file again
  File UploadFile = fs.open(filename, FILE_WRITE);  // Open the file for writing in SPIFFS (create it, if doesn't exist)
 // filename = String();
  if(!UploadFile){
        Serial.println("Failed to open file for writing");
        return;
  } else {

    fb = esp_camera_fb_get();

    if (!fb) {
      Serial.println("camera capture failed");
      return;
    } else {
      Serial.println("saving camera capture");
      if(UploadFile) {
        UploadFile.write(fb->buf, fb->len); // Write the received bytes to the file
        delay(1000);

          
          esp_camera_fb_return(fb);
          UploadFile.close();


          UploadFile = fs.open(filename);
          snprintf(str, sizeof str, "%zu", UploadFile.size());
          webpage = "";
          append_page_header();
          webpage += F("<h3>Snapshot was successfully taken</h3>"); 
          webpage += F("<h2>Uploaded File Name: "); webpage += filename +"</h2>";
          webpage += F("<h2>File Size: "); 
          webpage += str;
          webpage += F("</h2><br>");
          append_page_footer();
          
      
      } else {
        Serial.println("No snapshot file.... failed");
          webpage = "";
          append_page_header();
          webpage += F("<h3>Snapshot was NOT  taken</h3>"); 
          webpage += F("<h2>Uploaded File Name: "); webpage += filename +"</h2>";
          webpage += F("<h2>File Size: "); 
          webpage += str;
          webpage += F("</h2><br>");
          append_page_footer();
      }
      if(UploadFile) {
          Serial.println("wrote the upload file");
          Serial.print("buffer Size: ");Serial.println(fb->len);
          Serial.print("Upload Size: ");Serial.println(UploadFile.size());
        UploadFile.close();
        
      }

      
    }
 }
 server.send(200,"text/html",webpage);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Upload(){
  append_page_header();
  webpage += F("<h3>Select File to Upload</h3>"); 
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='buttons' style='width:40%' type='file' name='fupload' id = 'fupload' value=''><br>");
  webpage += F("<br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  server.send(200, "text/html",webpage);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
File UploadFile; 
void handleFileUpload(){ // upload a new file to the Filing system
  HTTPUpload& uploadfile = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
                                            // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if(uploadfile.status == UPLOAD_FILE_START)
  {
    String filename = uploadfile.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("Upload File Name: "); Serial.println(filename);
    SD_MMC.remove(filename);                         // Remove a previous version, otherwise data is appended the file again
    UploadFile = SD_MMC.open(filename, FILE_WRITE);  // Open the file for writing in SPIFFS (create it, if doesn't exist)
    filename = String();
  }
  else if (uploadfile.status == UPLOAD_FILE_WRITE)
  {
    if(UploadFile) UploadFile.write(uploadfile.buf, uploadfile.currentSize); // Write the received bytes to the file
  } 
  else if (uploadfile.status == UPLOAD_FILE_END)
  {
    if(UploadFile)          // If the file was successfully created
    {                                    
      UploadFile.close();   // Close the file again
      Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);
      webpage = "";
      append_page_header();
      webpage += F("<h3>File was successfully uploaded</h3>"); 
      webpage += F("<h2>Uploaded File Name: "); webpage += uploadfile.filename+"</h2>";
      webpage += F("<h2>File Size: "); webpage += file_size(uploadfile.totalSize) + "</h2><br>"; 
      append_page_footer();
      server.send(200,"text/html",webpage);
    } 
    else
    {
      ReportCouldNotCreateFile("upload");
    }
  }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Header(){
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
  server.sendHeader("Pragma", "no-cache"); 
  server.sendHeader("Expires", "-1"); 
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves. 
  append_page_header();
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Content(){
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Stop(){
  server.sendContent("");
  server.client().stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SelectInput(String heading1, String command, String arg_calling_name){
  SendHTML_Header();
  webpage += F("<h3>"); webpage += heading1 + "</h3>"; 
  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
  webpage += F("<input type='text' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
  webpage += F("<type='submit' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
  webpage += F("<a href='/'>[Back]</a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportSDNotPresent(){
  SendHTML_Header();
  webpage += F("<h3>No SD Card present</h3>"); 
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportFileNotPresent(String target){
  SendHTML_Header();
  webpage += F("<h3>File does not exist</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportCouldNotCreateFile(String target){
  SendHTML_Header();
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
String file_size(int bytes){
  String fsize = "";
  if (bytes < 1024)                 fsize = String(bytes)+" B";
  else if(bytes < (1024*1024))      fsize = String(bytes/1024.0,3)+" KB";
  else if(bytes < (1024*1024*1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
  else                              fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
  return fsize;
}
