#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

char apSsid[] = "GreenThumbTracker";       
char apPass[] = "password";

struct userdetails{
  String ssid;
  String password;
  String name;
  String serverurl;
};

ESP8266WebServer server(80);

void setup() {
Serial.begin(115200);
EEPROM.begin(512);
pinMode(0, OUTPUT);
  if (credentialsExist()){
    Serial.println("\nCredentials found");
    connectStation();
  }else{
    Serial.println("\nNo credentials, starting AP");
    initAP();
  }
}

void loop() {
  digitalWrite(0,HIGH);
  delay(2000);
  Serial.println(analogRead(A0));
  delay(1000);
  digitalWrite(0,LOW);
  delay(5000);

}

void connectStation(){
  Serial.print("Trying to connect...");
  userdetails user = getcredentials();
  WiFi.begin(user.ssid,user.password);
  while (!WiFi.isConnected()){
    delay(1000);
  }
  Serial.println("Successfully Connected ");
  Serial.println(WiFi.localIP());
  
}

void initAP(){
  Serial.println("Creating AP");
  WiFi.softAP(apSsid, apPass);
  delay(5000);
  server.on("/",handleBody);
  server.begin();
  Serial.print("AP created with SSID: ");
  Serial.println(apSsid);
  Serial.println("Listining for requests");
  while (!credentialsExist()){
    server.handleClient();   
  }
  
}

void handleBody(){
  Serial.println("Request received");
  Serial.printf("Stations connected to soft-AP = %d\n", WiFi.softAPgetStationNum());

  StaticJsonDocument<200> doc;

  if (server.hasArg("plain")== false){ //Check if body received
    server.send(404, "text/plain", "Body not received");
    return;
  }
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  if (error) {
    Serial.println(server.arg("plain"));
    Serial.print(("deserializeJson() failed: "));
    Serial.println(error.f_str());
    server.send(404, "text/plain", "Bad Data");
    return;
  }
  WiFi.begin(doc["ssid"].as<String>(),doc["password"].as<String>());
  while (!WiFi.isConnected()&&(WiFi.status()!=WL_CONNECT_FAILED && WiFi.status()!=WL_WRONG_PASSWORD && WiFi.status()!=WL_NO_SSID_AVAIL)){
     delay(500);
  }
  if (WiFi.status()==WL_CONNECTED){
    Serial.println("Sucessfully connected to new Wifi with IP: ");
    Serial.println(WiFi.localIP());
    server.send(200, "text/plain", "Correct credentials recieved");
    writeCredentials(doc);
    WiFi.softAPdisconnect(true);
  }else{
    server.send(401, "text/plain", "Incorrect credentials recieved");
    Serial.println("Incorrect Wifi credentials, try again");
  }
}

void writeCredentials(StaticJsonDocument<200> doc){
  for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
    }
  userdetails user = {
    doc["ssid"].as<String>(),doc["password"].as<String>(),doc["name"].as<String>(),doc["serverurl"].as<String>()
  };
  EEPROM.put(0,user);
  EEPROM.commit();
  delay(1000);

}

userdetails getcredentials(){
  userdetails user;
  EEPROM.get(0,user);
  delay(1000);
  return user;
}

bool credentialsExist(){
  userdetails user;
  EEPROM.get(0,user);
  delay(1000);
  if(user.ssid=="" && user.password==""){
    return false;
  }else{
    return true;
  }
}
