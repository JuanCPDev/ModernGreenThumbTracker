#include <CronAlarms.h>
#include <time.h>
#ifdef ESP8266
#include <sys/time.h>                   // struct timeval
#endif   
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>


char apSsid[] = "GreenThumbTracker";
char apPass[] = "password";
String connectionStatus = "No information recived";

struct userdetails {
  String ssid;
  String password;
  String name;
  String serverurl;
};

userdetails globaluser;

AsyncWebServer server(80);
bool serverdatasent;
bool credentialExist;


void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  serverdatasent=false;
  //Cron.create("*/2 * * * * *", updateServer, false);
  connectionStatus = "No information recived";
  credentialExist = credentialsExist();
  delay(1000);
  pinMode(0, OUTPUT);
  if (credentialExist) {
    connectionStatus = "Trying to connect using saved credentials";
    Serial.println("\nCredentials found");
    connectStation();
  } else {
    Serial.println("\nNo credentials, starting AP");
    initAP();
  }
}

void loop() {
  if (!WiFi.isConnected() && WiFi.status() == WL_WRONG_PASSWORD) {
    connectionStatus = "incorrect credentials";
  }
  if (WiFi.isConnected() && !credentialExist) {
    writeCredentials(globaluser);
    Serial.println("Connected to Wifi");
    connectionStatus = "Connected";
    credentialExist = credentialsExist();
    addTrackerInitialServer();
    delay(10000);
    server.end();
    WiFi.softAPdisconnect(true);
  }
  
}

void addTrackerInitialServer(){
  Serial.println("Adding new tracker to DB");
  WiFiClient client;
  HTTPClient http;
  String data= "{\"name\":\""+ globaluser.name +"\"}";
  http.begin(client,"url");
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.PUT(data);
  Serial.println("sent: "+data);
  Serial.println(httpResponseCode);
}

void checkMoisture(){
  /* TO DO
  Serial.println("Adding new tracker to DB");
  WiFiClient client;
  HTTPClient http;
  String data= "{\"name\":\""+ globaluser.name +"\"}";
  http.begin(client,"url");
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.PUT(data);
  Serial.println("sent: "+data);
  Serial.println(httpResponseCode);
  */
}

void connectStation() {
  Serial.print("Trying to connect...");
  globaluser = getcredentials();
  WiFi.begin(globaluser.ssid, globaluser.password);
  while (!WiFi.isConnected()) {
    delay(1000);
  }
  Serial.println("Successfully Connected ");
  connectionStatus = "Connected";
  Serial.println(WiFi.localIP());
}

void initAP() {
  Serial.println("Creating AP");
  WiFi.softAP(apSsid, apPass);
  delay(5000);
  Serial.print("AP created with SSID: ");
  Serial.println(apSsid);
  Serial.println("Listining for requests");

  server.on("/credentials", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (request->hasParam("ssid") && request->hasParam("password") && request->hasParam("serverurl") && request->hasParam("name")) {
      globaluser.ssid = request->getParam("ssid")->value();
      globaluser.password = request->getParam("password")->value();
      globaluser.serverurl = request->getParam("serverurl")->value();
      globaluser.name = request->getParam("name")->value();
      request->send(200, "text/plain", "Parameters recieved");
      connectionStatus = "Trying to connect...";
      WiFi.begin(globaluser.ssid, globaluser.password);

    } else {
      request->send(400, "text/plain", "Missing parameters");
    }
  });

  server.on("/getstatus", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", connectionStatus);
  });
  server.begin();
}

void writeCredentials(userdetails user) {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.put(0, user);
  EEPROM.commit();
  delay(1000);
}

userdetails getcredentials() {
  userdetails user;
  EEPROM.get(0, user);
  delay(1000);
  return user;
}

bool credentialsExist() {
  userdetails user;
  EEPROM.get(0, user);
  delay(1000);
  if (user.ssid == "" && user.password == "") {
    return false;
  } else {
    return true;
  }
}
