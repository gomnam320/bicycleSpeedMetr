#include <ESP8266WebServer.h>
#include <Arduino_JSON.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <LittleFS.h>
#include <assert.h>
#include <string.h>
#include <FS.h>
#include <Arduino.h>
#include <TM1637Display.h>

// Module connection pins (Digital Pins)
#define CLK D2
#define DIO D1
// Create a display object of type TM1637Display
TM1637Display display = TM1637Display(CLK, DIO);
// Create an array that turns all segments ON
const uint8_t allON[] = {0xff, 0xff, 0xff, 0xff};
// Create an array that turns all segments OFF
const uint8_t allOFF[] = {0x00, 0x00, 0x00, 0x00};
// Create an array that sets individual segments per digit to display the word "dOnE"
const uint8_t done[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
};
// Create degree celsius symbol
const uint8_t celsius[] = {
  SEG_A | SEG_B | SEG_F | SEG_G,  // Degree symbol
  SEG_A | SEG_D | SEG_E | SEG_F   // C
};

#ifndef APSSID
#define APSSID "Hossein"
#define APPSK "HosseinJanam"
#endif

File fsUploadFile;
JSONVar myObj;
ESP8266WebServer server(80);
String getContentType(String filename) {
  if (server.hasArg("download"))       return "application/octet-stream";
  else if (filename.endsWith(".htm"))  return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css"))  return "text/css";
  else if (filename.endsWith(".js"))   return "application/javascript";
  else if (filename.endsWith(".json")) return "application/json";
  else if (filename.endsWith(".wav"))  return "audio/wav";
  else if (filename.endsWith(".png") ) return "image/png";
  else if (filename.endsWith(".gif"))  return "image/gif";
  else if (filename.endsWith(".jpg"))  return "image/jpeg";
  else if (filename.endsWith(".ico"))  return "image/x-icon";
  else if (filename.endsWith(".xml"))  return "text/xml";
  else if (filename.endsWith(".pdf"))  return "application/x-pdf";
  else if (filename.endsWith(".zip"))  return "application/x-zip";
  else if (filename.endsWith(".gz"))   return "application/x-gzip";
  return "text/plain";
}
bool handleFileRead(String path) {

  //if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}
void handleFileUpload() {
  if (server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;

    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {

    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();

  }

  /*server.send(200, "text/html",
    //"upload complited successfull.</br>FileUpload Size: "+ String (upload.totalSize) + " <br>"
    "upload completed successfully.<br>"
    "<a href='./fileman'>Go to File Manager</a></br>powered by esp8266project.ir");*/
}
void handleFileDelete() {
  if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);

  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}
void handleFileCreate() {
  if (server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);

  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  File file = SPIFFS.open(path, "w");
  if (file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  server.send(200, "text/plain", "");
  path = String();
}
void handleFileList() {
  if (!server.hasArg("dir")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }

  String path = server.arg("dir");
  //DBG_OUTPUT_PORT.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while (dir.next()) {
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  server.send(200, "text/json", output);
}
void handleFilemanager() {                         /*////////////////////////////////////////Basic File Manager///////////////////////////////////////*/
  String output = "";
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      fileName = fileName.c_str();
      String SfileSize = String(fileSize);

      //output +=  fileName + " Size" + SfileSize + "</br>";
      output += "<a href='./remove?dir=" + fileName + "'>Remov</a> -- <a href='." + fileName + "'>" + fileName + "</a>  -------------  Size: " + SfileSize + " Bytes</br>";
    }
  }

  server.send(200, "text/html",
              "<form action='./edit' method='post' enctype='multipart/form-data'>"
              "file Manager<br><input type='file' name='data' >"
              "<input type='submit' value='Upload'></form>" + output + "</br><a href='./formatSPIFFS'>Delet All Files(Format SPIFFS)</a></hr>powered by hamim-elc.ir"
             );
}
void handle_format() {
  if (SPIFFS.format() == true) {
    server.send(200, "text/plain", "Format Compled");
  }
  else {
    server.send(200, "text/plain", "Error");
  }
}
void file_write() {
  if (server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");

  String filename = server.arg(0);
  String String_data = server.arg(1);
  if (!filename.startsWith("/")) {
    filename = "/" + filename;
  }
  File file = SPIFFS.open(filename, "w");
  if (file) {
    file.println(String_data);
  }
  file.close();
  server.send(200, "text/plain", "ok");
}
void handleRoot() {
  server.send(200, "text/html", "<h1>Salam Dashy</h1><br><h2>You are connected</h2>");
}

int Round = 0;
void saveData() {
  String String_data = JSON.stringify(myObj);
  File file = SPIFFS.open("/bicycle.json", "w");
  if (file) {
    file.println(String_data);
    Round = myObj["round"];
  }
  file.close();
}

void readData() {
  File file = SPIFFS.open("/bicycle.json", "r");
  if (file) {
    myObj = JSON.parse(file.readString());
    Round = myObj["round"];
  }
  file.close();
}
IPAddress local_IP(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
const char *password = APPSK;
const char *ssid = APSSID;

long int  oldT = 0, newT = 0;
short int oldS = 0, newS = 0;
bool timeing = 0;
int timeToSpeed(int Time = 0) {
  myObj["round"] = (int)myObj["round"] + 1;
  newS = (7464.42 / Time);
  if ((int)myObj["maxSpeed"] < newS) {myObj["maxSpeed"] = newS;saveData();}
  if (((int)myObj["round"] - Round) >= 50) {saveData();}
  Serial.println(newS);
  return newS;
}

boolean button = false;
void setup() {
  pinMode(D0, INPUT);
  
  display.setBrightness(2);
  display.setSegments(allON);
  display.clear();

  myObj["maxSpeed"] = 0;
  myObj["round"] = 0;
  if(SPIFFS.exists("/bicycle.json")) {readData();}
  
  Serial.begin(9600);
  Serial.println("\nConfiguring access point...");
  
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_IP, local_IP, subnet);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {}
  });
  server.on("/", handleRoot);
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  server.on("/edit", HTTP_PUT, handleFileCreate);
  server.on("/remove", HTTP_GET , handleFileDelete);
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);
  server.on("/fileman" , handleFilemanager);
  server.on("/formatSPIFFS", handle_format);
  server.on("/filewrite", HTTP_POST , file_write);
  server.begin();

  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  delay(1);
  if (digitalRead(D0) == 1 && timeing == 0) {
    newT = millis();
    timeing = 1;
    display.clear();
    display.showNumberDec(timeToSpeed(newT - oldT), false, 2, 1);
    oldT = newT;
  }
  if (digitalRead(D0) == 0 && timeing == 1) {
    timeing = 0;
  }
}
