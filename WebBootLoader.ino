/* 
* Este firmware para ESP8266 y ESP32 permite la carga del firmware
* y de los archivos en el sistema de ficheros SPIFFS a traves de
* un cliente WEB mediante protocolo HTTP.
* El uControlador publica una red WiFi donde el cliente WEB debe
* conectarse. El SSID de la red WiFi es "WebBootLoader" y la IP de
* uControlador es la 192.168.4.1
*/

#include <ESPAsyncWebServer.h> //Libreria para web server asincrono

#ifdef ESP32
  #include <WiFi.h>             //Añado la libreria de gestion del protocolo WiFi
  #include <SPIFFS.h>           //Libreria funciones SPIFFS
  #include <Update.h>          //Libreria para actualizacion SW via Web
  #define U_PART U_SPIFFS      //Constante para la actualizacion SW via Web
#endif
#ifdef ESP8266
  #include <FS.h>               //Libreria funciones SPIFFS
  #include <ESP8266WiFi.h>      //Añado la libreria de gestion del protocolo WiFi
  #include <Updater.h>          //Libreria para actualizacion SW via Web
  #define U_PART U_FS           //Constante para la actualizacion SW via Web
#endif

String fileType;                //Variable para almacenar el tipo de fichero recibido, esta variable debe ser global
File progFile;                  //Variable para almacenar el fichero recibido, esta variable debe ser global

const char* ssid = "WebBootLoader";
const char* pass = "qwertyuiop";

AsyncWebServer server(80);

void setup() {

  Serial.begin(115200);        //Inicializacion del puerto serie
  delay(1000);                //Retardo para establecer la conexion por el puerto serie
  WiFi.mode(WIFI_AP);         //Configuracion de la WiFi con Access Point
  WiFi.softAP(ssid, pass);    //Inicialización de la WiFi
  delay(5000);               //Tiempo de estabilizacion de la conexion WiFi
  Serial.println();
  Serial.println("*************************************************************");
  Serial.println("** Con tu PC conectate a la red WiFi \"WebBootLoader\"       **");
  Serial.println("** con password \"qwertyuiop\"                               **");
  Serial.println("** Abre el navegador e introduce la URL http://192.168.4.1 **");
  Serial.println("** Sube los ficheros a cargar                              **");
  Serial.println("** Reinicia con http://192.168.4.1/REBOOT                  **");
  Serial.println("*************************************************************");
   
  inicializacionWebServer();    //Inicializamos el servidor Web
  
}

void loop() {

}

void inicializacionWebServer()
{
   server.on("/REBOOT", [](AsyncWebServerRequest *request)
   {
      request->send(200, "text/plain", "Rebooting");
      delay(1000);
      ESP.restart();
   });
   
   server.on("/", HTTP_GET, htmlWebBootLoader);
   server.on("/WebBootLoader", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "Updated");
    }, handleFile);
  

   server.onNotFound([](AsyncWebServerRequest *request) {
      request->send(400, "text/plain", "Not found");
   });
 
   server.begin();
   Serial.println("HTTP server started");
}

void htmlWebBootLoader(AsyncWebServerRequest *request) {
  char* html = "<form method='POST' action='/WebBootLoader' enctype='multipart/form-data'><input type='file' name='codigo' multiple><input type='submit' value='Upload'></form>";
  request->send(200, "text/html", html);
}

void handleFile(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){

  if (!index){
    Serial.printf("UploadStart: %s\n", filename.c_str());
    fileType = (filename.indexOf("bin") >-1) ? "sketch" : "data";
    Serial.printf("The file type is %s\n", fileType);
    if (fileType == "data")
    {
      if (!SPIFFS.begin())
      {
        Serial.printf("\nError: al montar el sistema de ficheros.\n");
      }
      progFile = SPIFFS.open("/" + filename, "w+");
      if (!progFile)
      {
        Serial.printf("\nError: al abrir el fichero /%s en modo escritura.\n", filename.c_str());
      }
      if (progFile)
      {
        Serial.printf("\nSe ha abierto el fichero /%s en modo escritura.\n", filename.c_str());
      }
    }
    if (fileType == "sketch")
    {
      #ifdef ESP8266
      Update.runAsync(true);
      #endif
      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
      {
        Update.printError(Serial);
      }
    }
  }
  if (fileType == "data")
  {
    if (progFile.write(data, len) != len)
    {
      Serial.println("File write failed");
    }
  }
  if (fileType == "sketch")
  {
    if(!Update.hasError())
    {
      if(Update.write(data, len) != len)
      {
        Update.printError(Serial);
      }
    }
  }
  
  if(final)
  {
    if (fileType == "data")
    {
      progFile.close();
      Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index+len);
    }
    if (fileType == "sketch")
    {
      if(Update.end(true))
      {
        Serial.printf("Update Success: %uB\n", index+len);
      }
      else
      {
        Update.printError(Serial);
      }
    }
  }
}
