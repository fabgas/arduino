#include <DHTesp.h>
#define DHTTYPE DHT11 

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
DHTesp  dht;

#define DHT11_PIN 2
const char* ssid = "xxxxxx";           // Identifiant WiFi
const char* password = "xxxxxxxxx";// Mot de passe WiFi
ESP8266WebServer server(80);         // On instancie un serveur qui ecoute sur le port 80

int led =16;

void setup() {
  Serial.begin(115200);
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  Serial.println("");
   dht.setup(14, DHTesp::DHT11); // Connect DHT sensor to GPIO 0
 
  // on attend d'etre connecte au WiFi avant de continuer
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // on affiche l'adresse IP attribuee pour le serveur DSN
  Serial.println("");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // on definit les points d'entree (les URL a saisir dans le navigateur web) et on affiche un simple texte 
  server.on("/", [](){
    server.send(200, "text/plain", "Page d'accueil");
  });
  server.on("/on", [](){
    onLed();
    server.send(200, "text/plain", "on");
  });
  server.on("/off", [](){
    offLed();
    server.send(200, "text/plain", "Off");
  });
  // on demarre le serveur web 
  server.begin();
  pinMode(led, OUTPUT);     // Initialise la broche "led" comme une sortie - Initialize the "LED" pin as an output
}

// Cette boucle s'exécute à l'infini - the loop function runs over and over again forever
void loop() {
  // a chaque iteration, la fonction handleClient traite les requetes 
  server.handleClient();
    delay(2000);

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temperature, 1);
  Serial.print("\t\t");
  Serial.print(dht.toFahrenheit(temperature), 1);
  Serial.print("\t\t");
  Serial.print(dht.computeHeatIndex(temperature, humidity, false), 1);
  Serial.print("\t\t");
  Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true), 1);
  delay(2000);
}

void onLed() {
  digitalWrite(led, LOW);
}
void offLed() {
  digitalWrite(led, HIGH);
}
