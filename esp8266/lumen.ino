//
// ESP8266 based hardware, e.g. ADAFRUIT HUZZAH ESP8266 BREAKOUT
// https://www.adafruit.com/product/2471
// Reading the Analog Port and sending messgages via HTTP POST,
// if certain thresholds are passed
//

#include <ESP8266WiFi.h>

// WIFI Credentials
const char* ssid     = "barona9";
const char* password = "********";

// HTTP ADDRESS
const char* host = "intuitwear.intuitlabs.com";
const String path = "/iot/rest/forward";
const int httpPort = 80;
const String sid = "********";

// PORTS
const int sensorPin = A0;
const int ledPin = 0;

// THRESHOLDS
const int d1 = 50;
const int d2 = 100;
const int d3 = 150;

// 0 .. darkness .. natural .. aritifical
const int threshold0 = 10; // darkness
const int threshold1 = 250; // artificial
const int variance = 15;

// Global Vars
int k0, k1; // last two readings
String groups = NULL;
String cond  = NULL;
String icon  = NULL;


// Setup, runs once, on boot
void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  delay(10);

  // Connecting to a WiFi network
  Serial.println();
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Poll the analog sensor and only return,
// if thresholds are met
void sensorLoop() {
  k0 = k1 = analogRead(sensorPin);
  while (!interpretData(k0, k1)) {
    digitalWrite(ledPin, HIGH);
    delay(1000);
    digitalWrite(ledPin, LOW);
    k1 = analogRead(sensorPin);
    Serial.print("Sensor Value: ");
    Serial.println(k1);
  }
}

// Interpret sensor data and return true on sig. change
boolean interpretData(int k0, int k1) {
  int d = abs(k1 - k0);
  if (d < variance) {
    return false;
  }

  if (d >= d1) {
    groups = "d1";
    cond = "Small%20Fluctuation";
    icon = "r";
  }
  if (d >= d2) {
    groups = "d2";
    cond = "Medium%20Fluctuation";
  }
  if (d >= d3) {
    groups = "d3";
    cond = "Large%20Fluctuation";
  }

  if (k0 < threshold1 &&  threshold1 < k1) {
    // natural to artificial
    groups = "artificial";
    icon = "y";
    cond = "Artificial%20Lighting";
  } else if (k0 > threshold1 && threshold1 > k1) {
    // artificial to natural
    groups = "natural";
    icon = "g";
    cond = "Natural%20Lighting";
  } else if (k0 < threshold0 && threshold0 < k1) {
    // dark to natural or artificial
    if (k1 < threshold1) {
      groups = "natural";
      icon = "g";
      cond = "Natural%20Lighting";
    } else {
      groups = "artificial";
      icon = "y";
      cond = "Artificial%20Lighting";
    }
  } else if (k0 > threshold0 && threshold0 > k1) {
    // natural or artificial to darkness
    groups = "dark";
    cond = "Absence%20of%20Light";
  }
  k0 = k1;
  Serial.println(groups);
  return (groups != NULL);
}

// Send important change in sensor reading to remote server
// using the WiFiClient class to create a TCP connections
void sendNotification() {
  WiFiClient client;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // Create a URI for the request
  String url = path;
  url += "?sid=" + sid;
  url += "&grp=a_" + groups;
  url += "&title=ESP%20" + cond;
  url += "&text=Lighting%20conditions%20changed%20from%20" + String(k0) + "%20to%20" + String(k1);
  url += "&icon=" + icon;

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // Send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(10);

  // Read the reply from the server and print to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  Serial.println();
  Serial.println("closing connection");
}

// Main loop, send sig. changes in sensor readings to a remote server
void loop() {
  sensorLoop();
  sendNotification();
}
