#include <ESP8266WiFi.h>
#include <Losant.h>

// WiFi credentials.
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

// Losant credentials.
const char* LOSANT_DEVICE_ID = "";
const char* LOSANT_ACCESS_KEY = "";
const char* LOSANT_ACCESS_SECRET = "";

// Distance Sensor Pins
const int TRIG_PIN = 5;
const int ECHO_PIN = 4;

WiFiClient wifiClient;

LosantDevice device(LOSANT_DEVICE_ID);

void connectWifi() {
  Serial.println("`connectWifi`");

  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);

   // WiFi fix: https://github.com/esp8266/Arduino/issues/2186
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
void ensureWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    connectWifi();
  }
}

void connectLosant() {
  Serial.println("`connectLosant`");

  int timeout = 0;

  Serial.print("Connecting...");
  device.connect(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);

  while(!device.connected() || (timeout > 30 * 1000)) {
    delay(500);
    timeout += 500;
    Serial.print(".");
  }
  Serial.println("Connected!");
}
void ensureLosant() {
  if (!device.connected()) {
    Serial.println("Disconnected from MQTT");
    Serial.println(device.mqttClient.state());
    connectLosant();
  }
}

void disconnectLosant() {
  Serial.println("`disconnectLosant`");
  device.loop();
  delay(10);
  device.disconnect();
}

double readDistance() {
  long duration;
  double distance;

  // Clears the TRIG_PIN
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Sets the TRIG_PIN on HIGH state for 10 micro seconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Reads the ECHO_PIN, returns the sound wave travel time in microseconds
  duration = pulseIn(ECHO_PIN, HIGH);

  // Delay after reading so consecutive reads aren't so close together
  delay(100);

  // Calculating the distance
  // distance = duration * 0.034 / 2; // cm
  distance = duration * 0.0133 / 2; // inches

  // Serial.print("read: ");
  // Serial.println(distance);

  return distance;
}

// TODO: Take mode of long
double calculateDistance() {
  Serial.println("`calculateDistance`");

  double sum = 0;
  int count = 0;
  while (count < 30) {
    sum += readDistance();
    count += 1;
  }

  double distance = sum / count;

  Serial.print("Distance: ");
  Serial.println(distance);

  return distance;
}

void reportDistance(double distance) {
  Serial.println("`reportDistance`");

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["distance"] = distance;

  Serial.println("`sendState`");
  device.sendState(root);
}

void sleep(long seconds) {
  int sleepTime = seconds * 1000;
  Serial.print("sleeping for ");
  Serial.print(sleepTime);
  Serial.println("...");
  delay(sleepTime);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("Running Distance Firmware.");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  connectWifi();
  connectLosant();
}

void loop() {
  Serial.println("");
  Serial.println("--- loop ---");

  ensureWifi();
  ensureLosant();

  device.loop();

  reportDistance(calculateDistance());

  sleep(10);
}
