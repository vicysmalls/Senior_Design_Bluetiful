#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>

// Music Maker Shield Setup
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

#define SHIELD_RESET -1  // VS1053 reset pin (unused!)
#define SHIELD_CS 7      // VS1053 chip select pin (output)
#define SHIELD_DCS 6     // VS1053 Data/command select pin (output)
#define CARDCS 4         // Card chip select pin
#define DREQ 3           // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

//
char ssid[] = "tufts_eecs";
char pass[] = "foundedin1883";

// char ssid[] = "LatinWay160";
// char pass[] = "42Sunset";

// const char communityBroker[] = "mqtt.eclipseprojects.io";
// const char computerBroker[] = "172.17.9.16";
const char broker[] = "xx.x.x.xx";
int port = 1883;
const char mqttUser[] = "mqtt-user";
const char mqttPass[] = "bluetifulpi";

// const char topic[] = "water_well/level/raw";
const char topic[] = "add/data";

int keyIndex = 0;             // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;  //connection status
WiFiServer server(80);        //server socket

WiFiClient client = server.available();

WiFiClient WiFiclient;
MqttClient mqttClient(WiFiclient);

int microphonePin = A5;
const int dataSize = 500;
int data[dataSize];

void setup() {
  Serial.begin(9600);
  pinMode(microphonePin, INPUT);
  while (!Serial)
    ;

  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));
  
   if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }

  // Set volume for left, right channels. lower numbers == louder volume!
  Serial.println(F("Adjusting volume..."));
  musicPlayer.setVolume(20, 20);
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);

  enable_WiFi();
  connect_WiFi();

  server.begin();
  printWifiStatus();
  connectMQTT();
}

//####################  MAIN LOOP  #######################//

void loop() {
  // mqtt_logic();
  mqttClient.poll();  // avoids being disconnected by the broker

  check_WiFi();  // make sure wifi is connected

  // Web server logic
  client = server.available();
  if (client) {
    printWEB();
  }
}

//########################################################//

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print(F("IP Address: "));
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print(F("signal strength (RSSI):"));
  Serial.print(rssi);
  Serial.println(F(" dBm"));

  Serial.print(F("To see this page in action, open a browser to http://"));
  Serial.println(ip);
}

void enable_WiFi() {
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println(F("Communication with WiFi module failed!"));
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println(F("Please upgrade the firmware"));
  }
}

void connect_WiFi() {
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print(F("Attempting to connect to SSID: "));
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    Serial.print(F("."));
    // wait 10 seconds for connection:
    delay(5000);
  }

  Serial.println(F("You're connected to the network"));
  Serial.println();
}

void check_WiFi() {
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print(F("Disconnected to Wifi"));
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    Serial.print(F("."));
    // wait 10 seconds for connection:
    delay(5000);
  }
}

void connectMQTT() {
  Serial.print(F("Attempting to connect to the MQTT broker: "));
  Serial.println(broker);

  // mqttClient.setUsernamePassword(mqttUser, mqttPass);

  if (!mqttClient.connect(broker, port)) {
    Serial.print(F("MQTT connection failed! Error code = "));
    Serial.println(mqttClient.connectError());

    while (1)
      ;
  }
  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
}

void printWEB() {
  Serial.println(F("In Print Web"));
  if (client) {                    // if you get a client,
    Serial.println("new client");  // print a message out the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    while (client.connected()) {   // loop while the client's connected
      if (client.available()) {    // if there's bytes to read from the client,
        char c = client.read();    // read a byte, then
        Serial.write(c);           // print it out the serial monitor
        if (c == '\n') {           // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {

            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // Start of HTML document
            client.println("<!DOCTYPE html>");
            client.println("<html>");
            client.println("<head>");
            client.println("<title>Arduino Testbench</title>");  // Change the website title here
            client.println("</head>");
            client.println("<body>");

            //create the buttons
            client.print("Click <a href=\"/H\">here</a> to collect speaker data<br><br><br>");

            int randomReading = analogRead(A1);
            client.print("Random reading from analog pin: ");
            client.print(randomReading);

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {  // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        if (currentLine.endsWith("GET /H")) {
          Serial.println(F("\n|||||| DATA SENT ||||||"));
          send_mqtt();
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void send_mqtt() {
  Serial.print("Sending message to topic: ");
  Serial.println(topic);

  // musicPlayer.playFullFile("/track001.mp3");
  // musicPlayer.startPlayingFile("/track002.mp3");

  Serial.println(F("Testting..."));

  collectData();

  Serial.println(F("COLLECTED dATA"));


  int numChunks = 6;
  int chunkSize = dataSize / numChunks;  // Divide into 4 parts
  int remainder = dataSize % numChunks;  // Handle uneven division


  for (int chunk = 0; chunk < numChunks; chunk++) {
    mqttClient.beginMessage(topic);


    mqttClient.print("[");


    int startIdx = chunk * chunkSize;
    int endIdx = startIdx + chunkSize;


    // Add the remainder to the last chunk
    if (chunk == numChunks - 1) {
      endIdx += remainder;
    }


    // Iterate through chunk and send each number
    for (int i = startIdx; i < endIdx; i++) {
      mqttClient.print(data[i]);  // Convert int to string and send
      if (i < endIdx - 1) {
        mqttClient.print(",");  // Add comma separator
      }
    }


    mqttClient.print("]");
    mqttClient.endMessage();


    delay(50);  // Small delay between publishes
  }
}

void collectData() {
  // get data from analog pin
  for (int i = 0; i < dataSize; i++) {
    data[i] = analogRead(microphonePin);
    // delayMicroseconds(53);
  }
}
