#include <ESP8266WiFi.h>
#include <SdFat.h>
#include <SPI.h>
#include <stdio.h>
#include <algorithm>
#include <WString.h>
#include <string>

// Credenciales del AP del ESP8266
const char* ssid = "ESP8266_SD_AP";
const char* password = "12345678";

// IP del ESP8266 (servidor)
IPAddress server(192, 168, 4, 1);

// Cliente WiFi
WiFiClient client;

// Configuración de SdFat
SdFat SD;
const uint8_t SD_CS_PIN = 10; // Cambia al pin CS que estés usando

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Conectar a WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a WiFi.");

  // Inicializar tarjeta SD usando SdFat
  if (!SD.begin(SD_CS_PIN, SD_SCK_MHZ(10))) {
    Serial.println("Error inicializando la tarjeta SD.");
    while (true);
  }
  Serial.println("Tarjeta SD inicializada.");
}

void loop() {
  // Solicitar lista de archivos
  if (client.connect(server, 80)) {
    client.println("GET /files HTTP/1.1");
    client.println("Host: 192.168.4.1");
    client.println("Connection: close");
    client.println();

    // Leer la lista de archivos
    String jsonResponse = "";
    while (client.connected() || client.available()) {
      if (client.available()) {
        jsonResponse += char(client.read());
      }
    }
    client.stop();

    // Procesar la respuesta JSON
    Serial.println("Respuesta JSON:");
    Serial.println(jsonResponse);
    jsonResponse = jsonResponse.substring(jsonResponse.indexOf("[") + 1, jsonResponse.indexOf("]"));
    while (jsonResponse.length() > 0) {
      int delimiter = jsonResponse.indexOf(",");
      String fileName = jsonResponse.substring(0, delimiter);
      fileName.replace("\"", ""); // Quitar comillas
      if (delimiter == -1) {
        fileName = jsonResponse;
        jsonResponse = "";
      } else {
        jsonResponse = jsonResponse.substring(delimiter + 1);
      }
      
      if (fileName != "System Volume Information"){
        digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on (Note that LOW is the voltage level
        downloadFile(fileName);
        digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH

      }
    }
  } else {
    Serial.println("Error conectando al servidor.");
  }

  delay(10000); // Esperar antes de repetir
}

void downloadFile(const String& fileName) {
  if (client.connect(server, 80)) {
    // Solicitar el archivo
    client.print("GET /download/");
    client.print(fileName);
    client.println(" HTTP/1.1");
    client.println("Host: 192.168.4.1");
    client.println("Connection: close");
    client.println();

    // Crear el archivo en la tarjeta SD
    FsFile file = SD.open(fileName.c_str(), O_CREAT | O_WRITE);
    if (!file) {
      Serial.println("Error creando el archivo en la SD.");
      return;
    }

    // Descargar y guardar el archivo
    while (client.connected() || client.available()) {
      if (client.available()) {
        file.write(client.read());
      }
    }
    file.close();
    client.stop();
    Serial.println("Archivo descargado: " + fileName);
  } else {
    Serial.println("Error conectando al servidor para descargar " + fileName);
  }
}
