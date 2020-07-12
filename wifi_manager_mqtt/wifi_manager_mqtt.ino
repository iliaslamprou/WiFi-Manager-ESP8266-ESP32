/* ---------------------------------------------------------------
 ESP32 or ESP8266 WiFi & MQTT  Manager   ver 1.0.0  
 Using this code you can configure the WiFi SSID, the WiFi Password and the MQTT settings without re-compile the sketch
 How it works:
 1. Connect a button to the pin D6 for ESP6266 or 4 for ESP32
 2. Upload the code
 3. Push the button down for 2-3 seconds. The board will restart as Access Point
 4. Connect your phone to the board wifi network (ESP_Network)
 5. Open the browser, enter the ip: 192.168.4.1
 6. You have to see a web page. Use the web page to configure the SSID and Password
 7. Push the button again or restart the board. The board will connect to the new network

 Created by Ilias Lamprou at Jul/11/2020
 */
//---------------------------------------------------------------

#ifdef ESP8266
 #include <ESP8266WiFi.h>  // Pins for board ESP8266 Wemos-NodeMCU
 #include <ESP8266WebServer.h>
 ESP8266WebServer serverAP(80);
 #define accessPointButtonPin D6   // Connect a button to this pin
 #else
  #include <WiFi.h>
  #include <WebServer.h>
  #define accessPointButtonPin 4    // Connect a button to this pin
  WebServer serverAP(80);   // the Access Point Server
 #endif

    
#define accessPointLed LED_BUILTIN
#define eepromTextVariableSize 33  // the max size of the ssid, password etc.  32+null terminated

//---------- wifi default settings ------------------
char ssid[eepromTextVariableSize] = "WiFi Name";       
char pass[eepromTextVariableSize] = "WiFi Password";   


//------ MQTT broker settings and topics
char broker[eepromTextVariableSize] = "broker.shiftr.io"; 
char mqttUserName[eepromTextVariableSize] = "board_tester";         
char mqttPass[eepromTextVariableSize] = "1234567890";      


const char* topic_1 = "my_topic_1";    // published 
const char* topic_2 = "my_topic_2";    // published
const char* topic_3 = "my_topic_3";    // subscribed


#include <MQTTClient.h>
WiFiClient net;
MQTTClient client;
   
/* Enable these lines in case you want to change the default Access Point ip: 192.168.4.1. You have to enable the line:   
  WiFi.softAPConfig(local_ip, gateway, subnet); 
  on the void initAsAccessPoint too */
//IPAddress local_ip(192,168,1,1);
//IPAddress gateway(192,168,1,1);
//IPAddress subnet(255,255,255,0);



boolean accessPointMode= false;     // is true every time the board is started as Access Point
boolean debug = true;               
unsigned long lastUpdatedTime =0;

int pushDownCounter=0;
int lastConnectedStatus=0;

#include <EEPROM.h>
//================================================ initAsAccessPoint
void initAsAccessPoint(){
      WiFi.softAP("ESP_Network");      // or WiFi.softAP("ESP_Network","Acces Point Password"); 
      if (debug) Serial.println("AccesPoint IP: "+WiFi.softAPIP().toString());
      Serial.println("Mode= Access Point");
      //WiFi.softAPConfig(local_ip, gateway, subnet);  // enable this line to change the default Access Point IP address
      delay(100);
}

//========================================= connect
void mqttConnect() {
  Serial.print("\nconnecting to wifi.");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    checkIfModeButtonPushed();
  }
  //--- create a random client id
  char clientID[] ="ESP8266_0000000000";  // For random generation of client ID.
  for (int i = 8; i <18 ; i++) clientID[i]=  char(48+random(10));
    
  Serial.print("\nconnecting to broker...");
  while (!client.connect(clientID,mqttUserName,mqttPass)) {
    Serial.print(".");
    delay(500);
    checkIfModeButtonPushed();
  }
  Serial.println("\nconnected!");
  client.subscribe(topic_3);
  // client.unsubscribe(topic_2);
}


//================================================ setup
//================================================
void setup() {
  Serial.begin(115200);
  delay(500);
 
  //--- Check the first EEPROM byte. If this byte is "2" the board will start as Access Point
  int st=getStatusFromEeprom();
  if (st==2) accessPointMode=true;
  else if (st!=0) saveSettingsToEEPPROM(ssid,pass,broker, mqttUserName,mqttPass); // run the void saveSettingsToEEPPROM on the first running or every time you want to save the default settings to eeprom
  Serial.println("\n\naccessPointMode="+String(accessPointMode)); 

 readSettingsFromEEPROM(ssid,pass,broker, mqttUserName,mqttPass);    // read the SSID and Passsword from the EEPROM 
 
 if (accessPointMode) {     // start as Access Point
     initAsAccessPoint();
     serverAP.on("/", handle_OnConnect);
     serverAP.onNotFound(handle_NotFound);
     serverAP.begin();
    saveStatusToEeprom(0);  // enable the Client mode for the the next board starting     
  }
    else {            // start as client
      Serial.println("Mode= Client");
      WiFi.begin(ssid, pass);
      // Enter your client setup code here
    }

  client.begin(broker, net);
  client.onMessage(messageReceived);

  
  pinMode(accessPointButtonPin,INPUT);
  pinMode(accessPointLed,OUTPUT);
  
}

//========================================= messageReceived
void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
 /* if (topic==topic_3){
    int v = payload.toInt();
    if (v==1) digitalWrite(D4,HIGH);
    else digitalWrite(D4,LOW);
  }
*/  
}

//============================================== loop
//==============================================
void loop() {
  if (accessPointMode) {
    serverAP.handleClient();
    playAccessPointLed();       // blink the LED every time the board works as Access Point
  }
  else {
      client.loop();
      delay(10);  // <- fixes some issues with WiFi stability
      if (!client.connected())mqttConnect();

 
    // enter your client code here
      
    if (millis() - lastUpdatedTime > 5000) {
    int sensorValue_1=random(100);  // replace with your sensor value
    int sensorValue_2=random(100);  // replace with your sensor value
    client.publish(topic_1, String(sensorValue_1),true,1);
    client.publish(topic_2, String(sensorValue_1),true,1);
    lastUpdatedTime = millis();
  }



    
  }
  checkIfModeButtonPushed();
}

//============================================
//--- Mode selector Button - push down handler
 void checkIfModeButtonPushed(){
  while (digitalRead(accessPointButtonPin)){
    pushDownCounter++;
    if (debug) Serial.println(pushDownCounter);
    delay(100);
    if (pushDownCounter==20){     // after 2 seconds the board will be restarted
      if (!accessPointMode) saveStatusToEeprom(2);   // write the number 2 to the eeprom
      ESP.restart();
    }
  }
  pushDownCounter=0; 
 }

//============================================ playAccessPointLed
 unsigned long lastTime=0;
 void playAccessPointLed(){
    if (millis()-lastTime>300){
     lastTime=millis();
     digitalWrite(accessPointLed,!digitalRead(accessPointLed));
    }
 }
   
//================ WiFi Manager necessary functions ==============
//================================================================
//================================================================ 
  
//==============================================
void handle_OnConnect() {
  if (debug) Serial.println("Client connected:  args="+String(serverAP.args()));
  if (serverAP.args()>=2)  {
    handleGenericArgs(); 
    serverAP.send(200, "text/html", SendHTML(1)); 
  }
  else  serverAP.send(200, "text/html", SendHTML(0)); 
}

//==============================================
void handle_NotFound(){
  if (debug) Serial.println("handle_NotFound");
  serverAP.send(404, "text/plain", "Not found");
}

//=================================
void handleGenericArgs() { //Handler
  for (int i = 0; i < serverAP.args(); i++) {
    if (debug) Serial.println("*** arg("+String(i)+") ="+ serverAP.argName(i));
    if (serverAP.argName(i)=="ssid") {
     if (debug)  Serial.print("sizeof(ssid)=");Serial.println(sizeof(ssid));
      memset(ssid, '\0', sizeof(ssid));
      strcpy(ssid, serverAP.arg(i).c_str());
    }
    else   if (serverAP.argName(i)=="pass") {
      if (debug) Serial.print("sizeof(pass)=");Serial.println(sizeof(pass));
      memset(pass, '\0', sizeof(pass));
      strcpy(pass, serverAP.arg(i).c_str());
    }
    else   if (serverAP.argName(i)=="broker") {
      memset(broker, '\0', sizeof(broker));
      strcpy(broker, serverAP.arg(i).c_str());
    }
     else   if (serverAP.argName(i)=="mqtt_username") {
      memset(mqttUserName, '\0', sizeof(mqttUserName));
      strcpy(mqttUserName, serverAP.arg(i).c_str());
    }
     else   if (serverAP.argName(i)=="mqtt_pass") {
      memset(mqttPass, '\0', sizeof(mqttPass));
      strcpy(mqttPass, serverAP.arg(i).c_str());
    }
  }
  if (debug) Serial.println("*** New settings have received");
  if (debug) Serial.print("*** ssid=");Serial.println(ssid);
  if (debug) Serial.print("*** password=");Serial.println(pass);
  if (debug) Serial.print("*** broker=");Serial.println(broker);
  if (debug) Serial.print("*** mqttUserName=");Serial.println(mqttUserName);
  if (debug) Serial.print("*** mqttPass=");Serial.println(mqttPass);
  
  
  saveSettingsToEEPPROM(ssid,pass,broker,mqttUserName,mqttPass);
 
}
//===================================
String SendHTML(uint8_t st){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>ESP WiFi Manager</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 30px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +="label{display:inline-block;width: 160px;text-align: right;}\n";
  ptr +="form{margin: 0 auto;width: 380px;padding: 1em;border: 1px solid #CCC;border-radius: 1em; background-color: #3498db;}\n";
  ptr +="input {margin: 0.5em;}\n";
  if (st==1) ptr +="h3{color: green;}\n";
  ptr +="</style>\n";
  ptr +="<meta charset=\"UTF-8\">\n";   
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>ESP WiFi & MQTT Manager</h1>\n";
  if (st==1)ptr +="<h3>WiFi settings has saved successfully!</h3>\n";
  else if (st==2)ptr +="<h3>MQTT settings has saved successfully!</h3>\n";
  else ptr +="<h3>Enter the WiFi settings</h3>\n";
  
  //---- form1   SSID- Pasword
  ptr +="<form id=\"msform1\">";
  ptr +="<div><label for=\"label_1\">WiFi SSID</label><input id=\"ssid_id\" required type=\"text\" name=\"ssid\" value=\"";
  ptr +=ssid;
  ptr +="\" maxlength=\"32\"></div>\n";
  ptr +="<div><label for=\"label_2\">WiFi Password</label><input id=\"pass_id\" type=\"text\" name=\"pass\" value=\"";
  ptr +=pass;
  ptr +="\" maxlength=\"32\"></div>\n";
  ptr +="<div><input type=\"submit\" value=\"Send\"accesskey=\"s\"></div></form>";   
  

  //---- form2   MQTT settings
  ptr +="<form id=\"msform2\">";
  ptr +="<div><label for=\"label_3\">Broker url</label><input id=\"broker_id\" required type=\"text\" name=\"broker\" value=\"";
  ptr +=broker;
  ptr +="\" maxlength=\"32\"></div>\n";

  ptr +="<div><label for=\"label_4\">Username</label><input id=\"username_id\" required type=\"text\" name=\"mqtt_username\" value=\"";
  ptr +=mqttUserName;
  ptr +="\" maxlength=\"32\"></div>\n";
  
  ptr +="<div><label for=\"label_5\">MQTT Password</label><input id=\"mqtt_pass_id\" type=\"text\" name=\"mqtt_pass\" value=\"";
  ptr +=mqttPass;
  ptr +="\" maxlength=\"32\"></div>\n";
   
  ptr +="<div><input type=\"submit\" value=\"Send\"accesskey=\"s\"></div></form>";   

  ptr +="<h5>Created by Ilias Lamprou</h5>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}







//====================== EEPROM necessary functions ==============
//================================================================
//================================================================
#define eepromBufferSize 200     // have to be >  eepromTextVariableSize * (eepromVariables+1)   (33 * (5+1))

//========================================== writeDefaultSettingsToEEPPROM
void saveSettingsToEEPPROM(char* ssid_, char* pass_, char* broker_, char* mqttUsername_, char* mqttPass_){
  if (debug) Serial.println("\n============ saveSettingsToEEPPROM");
  writeEEPROM(1* eepromTextVariableSize , eepromTextVariableSize , ssid_);
  writeEEPROM(2* eepromTextVariableSize , eepromTextVariableSize ,  pass_);
  
  writeEEPROM(3* eepromTextVariableSize , eepromTextVariableSize ,  broker_);
  writeEEPROM(4* eepromTextVariableSize , eepromTextVariableSize ,  mqttUsername_);
  writeEEPROM(5* eepromTextVariableSize , eepromTextVariableSize ,  mqttPass_);
  
}
//========================================== readSettingsFromEeprom
void readSettingsFromEEPROM(char* ssid_, char* pass_, char* broker_, char* mqttUsername_, char* mqttPass_){
  readEEPROM( 1* eepromTextVariableSize , eepromTextVariableSize , ssid_);
  readEEPROM( (2* eepromTextVariableSize) , eepromTextVariableSize , pass_);
  
  readEEPROM( (3* eepromTextVariableSize) , eepromTextVariableSize , broker_);
  readEEPROM( (4* eepromTextVariableSize) , eepromTextVariableSize , mqttUsername_);
  readEEPROM( (5* eepromTextVariableSize) , eepromTextVariableSize , mqttPass_);
  
  
  if (debug) Serial.println("\n============ readSettingsFromEEPROM");
  if (debug) Serial.print("\n============ ssid=");if (debug) Serial.println(ssid_);
  if (debug) Serial.print("============ password=");if (debug) Serial.println(pass_);
  if (debug) Serial.print("============ broker=");if (debug) Serial.println(broker_);
  if (debug) Serial.print("============ mqttUsername=");if (debug) Serial.println(mqttUsername_);
  if (debug) Serial.print("============ mqttPassword=");if (debug) Serial.println(mqttPass_);
  
}

//================================================================
void writeEEPROM(int startAdr, int length, char* writeString) {
  EEPROM.begin(eepromBufferSize);
  yield();
  for (int i = 0; i < length; i++) EEPROM.write(startAdr + i, writeString[i]);
  EEPROM.commit();
  EEPROM.end();           
}

//================================================================
void readEEPROM(int startAdr, int maxLength, char* dest) {
  EEPROM.begin(eepromBufferSize);
  delay(10);
  for (int i = 0; i < maxLength; i++) dest[i] = char(EEPROM.read(startAdr + i));
  dest[maxLength-1]=0;
  EEPROM.end();}

 //================================================================ writeEepromSsid
 void saveStatusToEeprom(byte value){
   EEPROM.begin(eepromBufferSize);
   EEPROM.write(0,value);
   EEPROM.commit();
   EEPROM.end();     
 }
//===================================================================
 byte getStatusFromEeprom(){
   EEPROM.begin(eepromBufferSize);
   byte value=0;
   value =EEPROM.read (0);
   EEPROM.end();
   return value;
 }
 
