#include <ESP8266WiFi.h>
#include <SdFat.h>

// Pines para la tarjeta SD
#define CS_PIN 15 // Ajusta según tu conexión
#define pinBoton 16

// Credenciales del Access Point
const char* ssid = "ESP8266_SD_AP";
const char* password = "12345678";

// Crear servidor WiFi en el puerto 80
WiFiServer server(80);

// Crear una instancia de SdFat
SdFat SD;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(pinBoton, INPUT_PULLUP);

  // Configurar el Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("IP del AP: ");
  Serial.println(IP);
  WiFi.setOutputPower(17);      // Opcional: ajustar potencia de salida (en dBm)
  wifi_set_phy_mode(PHY_MODE_11N);  // Configurar PHY para usar 20 MHz en el AP

  // Inicializar la tarjeta SD
  if (!SD.begin(CS_PIN, SD_SCK_MHZ(10))) {
    Serial.println("Error inicializando la tarjeta SD.");
    while (true);
  }

  // Iniciar el servidor
  server.begin();
  Serial.println("Servidor iniciado.");
}

void listFilesJSON(WiFiClient& client) {
  client.print("{files:[");

  FsFile root = SD.open("/");
  if (!root || !root.isDirectory()) {
    client.print("]}");
    return;
  }

  FsFile file = root.openNextFile();
  bool firstFile = true;
  while (file) {
    char fileName[64];
    file.getName(fileName, sizeof(fileName));
    if (!firstFile) client.print(",");
    client.print(fileName);
    firstFile = false;
    file = root.openNextFile();
  }
  client.print("]}");
}

void downloadFile(WiFiClient& client, const String& fileName) {
  FsFile file = SD.open(fileName.c_str(), FILE_READ);
  if (!file) {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Archivo no encontrado.");
    return;
  }

  uint8_t buffer[512];
  int bytesRead;
  while ((bytesRead = file.read(buffer, sizeof(buffer))) > 0) {
    client.write(buffer, bytesRead);
  }

  file.close();
}

void loop() {
  int boton = digitalRead(pinBoton);
  if (boton == LOW) {
    Serial.println("boton no presionado");   
  }
  else {
    while(1){
        digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on (Note that LOW is the voltage level   
        Serial.println("boton presionado");   
        WiFiClient client = server.available();
        if (client) {
          String request = client.readStringUntil('\r');
          client.flush();

          if (request.startsWith("GET /files")) {
            listFilesJSON(client);
          } else if (request.startsWith("GET /download/")) {
            int startIndex = request.indexOf("/download/") + 10;
            int endIndex = request.indexOf(" ", startIndex);
            String fileName = request.substring(startIndex, endIndex);
            downloadFile(client, "/" + fileName);
          } else {
            client.println("HTTP/1.1 404 Not Found");
            client.println("Connection: close");
            client.println();
          }
          client.stop();
        }
        delay(1);
      }
  } 
}
