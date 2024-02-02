/*Sketch which publishes temperature data from a DS1820 sensor to a MQTT topic.
This sketch goes in deep sleep mode once the temperature has been sent to the MQTT
topic and wakes up periodically (configure SLEEP_DELAY_IN_SECONDS accordingly).
Hookup guide:
- connect D0 pin to RST pin in order to enable the ESP8266 to wake up periodically
- DS18B20:
+ connect VCC (3.3V) to the appropriate DS18B20 pin (VDD)
+ connect GND to the appopriate DS18B20 pin (GND)
+ connect D4 to the DS18B20 data pin (DQ)
+ connect a 4.7K resistor between DQ and VCC.
*/
#include<ESP8266WiFi.h>
#include<PubSubClient.h>
#include<OneWire.h>
#include<DallasTemperature.h>
#include<Streaming.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

#define SLEEP_DELAY_IN_SECONDS 600 // Aufwachen aus dem Deepsleep alle 10 Minuten

#define ONE_WIRE_BUS 2               // DS18B20 pin

// ADC
ADC_MODE (ADC_VCC);                     // VCC Read

// LED-Pin definieren
const int ledPin = 5;

//Static IP address configuration
IPAddress staticIP(192, 168, 1, 144); //ESP static ip
IPAddress gateway(192, 168, 1, 1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(8, 8, 8, 8);  //DNS
// Set web server port number to 80
WiFiServer server(80);

const char* ssid = "wlan-ssid";
const char* password = "wlan-password";
const char* Hostname = "hostname";
const char* mqtt_server = "IP-Address";
const char* mqtt_username = "mqtt-username";
const char* mqtt_password = "mqtt-password";
const char* mqtt_topic = "Aussensensor1/temperature";
const char* mqtt_topic_vcc = "Aussensensor1/vcc";
const char* mqtt_topic_akku = "Aussensensor1/Akku";
char msg[50];  // message für mqtt publish
float schwellwert = 2.6;  // Schwellwert für Auswertung Akku aufladen, sinkt der Wert vcc unter 2.6, dann sende mqtt-Nachricht "Aussensensor1/Akku" mit dem state "ON" true
//float schwellwert = 4; // testwert

WiFiClient espClient;
PubSubClient client(espClient);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
char temperatureString[6];

void setup() { 
  // setup serial port
  Serial.begin(115200); 
  // setup LED-Pin
  pinMode(ledPin, OUTPUT);
  // setup WiFi
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  // setup OneWire bus
  DS18B20.begin();
  }

void setup_wifi() {
  delay(10); // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.config(staticIP, subnet, gateway, dns);
  WiFi.hostname(Hostname);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
            }
    
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    }
    Serial.println();
    }
    
void reconnect() { // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection..."); // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds"); // Wait 5 seconds before retrying
        delay(5000);
        }
        }
        }
        float getTemperature() {
          Serial << "Requesting DS18B20 temperature..." << endl;
          float temp;
          do {
            DS18B20.requestTemperatures();
            temp = DS18B20.getTempCByIndex(0);
            delay(100);
            } while (temp == 85.0 || temp == (-127.0));
            return temp;
            }
            
void loop() {
  if (!client.connected()) {
    reconnect();
    }
    client.loop();
    // Read the VCC from Battery
    float vcc = ESP.getVcc() / 1000.0;
    vcc = vcc - 0.12; // correction value from VCC
    // Serial.print (vcc);
    dtostrf(vcc, sizeof(vcc), 2, msg);
    // LED einschalten, LED Leuchtet während des Sendevorganges
    digitalWrite(ledPin,HIGH);
    float temperature = getTemperature(); // convert temperature to a string with two digits before the comma and 2 digits for
    dtostrf(temperature, 2, 2, temperatureString); // send temperature to the serial console
    // Serial << "Sending temperature: " << temperatureString << endl; // send temperature to the MQTT topic
    client.publish(mqtt_topic, temperatureString);
    delay(50);
    client.publish(mqtt_topic_vcc, msg);
    delay(50);
    if (vcc <= schwellwert) {client.publish(mqtt_topic_akku, "ON", true);}
    else {client.publish(mqtt_topic_akku, "OFF", false);}
    // Serial << "Closing MQTT connection..." << endl;
    client.disconnect();
    // Serial << "Closing WiFi connection..." << endl;
    
    // LED ausschalten
    digitalWrite(ledPin,LOW);
    WiFi.disconnect();
    delay(100);
    //Serial << "Entering deep sleep mode for " << SLEEP_DELAY_IN_SECONDS <<
    ESP.deepSleep(SLEEP_DELAY_IN_SECONDS * 1000000, WAKE_RF_DEFAULT); //ESP.deepSleep(10 * 1000, WAKE_NO_RFCAL);
    delay(500); // wait for deep sleep to happen
    }
