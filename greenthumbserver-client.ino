#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

char apSsid[] = "GreenThumbTracker";
char apPass[] = "password";
String connectionStatus = "No information recived";
String host = "hostUrl";

struct userdetails
{
  char ssid[50];
  char password[50];
  char name[50];
  char userId[100];
  bool addedtodb;
};

userdetails globaluser;

AsyncWebServer server(80);
bool serverdatasent;
bool credentialExist;
bool trackerAddedToDb;

void writeCredentials(userdetails user)
{
  for (int i = 0; i < EEPROM.length(); i++)
  {
    EEPROM.write(i, 0);
  }
  EEPROM.put(0, user);
  EEPROM.commit();
}

int readValue()
{
  int value;
  int total = 0;
  int valueList[5];
  digitalWrite(5, HIGH);
  delay(500);
  valueList[0] = analogRead(A0);
  delay(500);
  valueList[1] = analogRead(A0);
  delay(500);
  valueList[2] = analogRead(A0);
  delay(500);
  valueList[3] = analogRead(A0);
  delay(500);
  valueList[4] = analogRead(A0);

  for (int in : valueList)
  {
    total = total + in;
  }
  value = total / 5;
  digitalWrite(5, LOW);
  return value;
}
userdetails getcredentials()
{
  userdetails user;
  EEPROM.get(0, user);
  return user;
}

void addTrackerInitialServer()
{
  Serial.println("Adding new tracker to DB");
  WiFiClient client;
  HTTPClient http;
  String str(globaluser.name);
  Serial.println(str);
  String addTrackerUrl = host + "addtracker?userId=" + globaluser.userId;
  String data = "{\"name\":\"" + str + "\"}";
  http.begin(client, addTrackerUrl);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.PUT(data);
  Serial.println("sent: " + data);
  Serial.println(httpResponseCode);
  http.end();
}

void sendValue(int value)
{
  WiFiClient client;
  HTTPClient http;
  String str(globaluser.name);
  String sendValueUrl = host + "updatetracker?userId=" + globaluser.userId;
  String data = "{\"name\":\"" + str + "\",\"value\":\"" + value + "\"}";
  http.begin(client, sendValueUrl);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.PUT(data);
  Serial.println("sent: " + data);
  Serial.println(httpResponseCode);
  http.end();
}

void connectStation()
{
  Serial.print("Trying to connect...");
  globaluser = getcredentials();
  WiFi.begin(globaluser.ssid, globaluser.password);
  while (!WiFi.isConnected())
  {
    delay(500);
  }
  Serial.println("Successfully Connected ");
  connectionStatus = "Connected";
  Serial.println(WiFi.localIP());
}

void initAP()
{
  Serial.println("Creating AP");
  WiFi.softAP(apSsid, apPass);
  delay(5000);
  Serial.print("AP created with SSID: ");
  Serial.println(apSsid);
  Serial.println("Listining for requests");

  server.on("/credentials", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    Serial.println("Request recieved");
    if (request->hasParam("ssid") && request->hasParam("password") && request->hasParam("userId") && request->hasParam("name")) {
      strcpy(globaluser.ssid, request->getParam("ssid")->value().c_str());
      strcpy(globaluser.password, request->getParam("password")->value().c_str());
      strcpy(globaluser.userId, request->getParam("userId")->value().c_str());
      strcpy(globaluser.name, request->getParam("name")->value().c_str());
      request->send(200, "text/plain", "Parameters recieved");
      connectionStatus = "Trying to connect...";
      WiFi.begin(globaluser.ssid, globaluser.password);

    } else {
      request->send(400, "text/plain", "Missing parameters");
    } });

  server.on("/getstatus", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", connectionStatus); });
  server.begin();
}

bool credentialsExist()
{
  userdetails user;
  EEPROM.get(0, user);
  if (user.ssid[0] == 0 && user.password[0] == 0)
  {
    return false;
  }
  else
  {
    return true;
  }
}

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(512);
  globaluser = getcredentials();
  serverdatasent = false;
  credentialExist = false;
  connectionStatus = "No information recived";
  credentialExist = credentialsExist();
  delay(1000);
  pinMode(5, OUTPUT);
  if (credentialExist)
  {
    connectionStatus = "Trying to connect using saved credentials";
    Serial.println("\nCredentials found");
    connectStation();
  }
  else
  {
    Serial.println("\nNo credentials, starting AP");
    initAP();
  }
}

void loop()
{
  if (!WiFi.isConnected() && WiFi.status() == WL_WRONG_PASSWORD)
  {
    connectionStatus = "incorrect credentials";
  }
  if (WiFi.isConnected() && !credentialExist)
  {
    globaluser.addedtodb = true;
    writeCredentials(globaluser);
    Serial.println("Connected to Wifi");
    connectionStatus = "Connected";
    credentialExist = credentialsExist();
    delay(10000);
    server.end();
    WiFi.softAPdisconnect(true);
    addTrackerInitialServer();
  }
  if (WiFi.isConnected() && credentialExist && globaluser.addedtodb)
  {
    int value = readValue();
    sendValue(value);
    delay(5000);
    ESP.deepSleep(3.6e+6); // one hour interval
    
  }
}