/* ---------------------------------------------------------------
 ESP32 or ESP8266 WiFi & IP  Manager (Virtuino Board Server)  ver 1.0.0  
 Using this code you can configure the WiFi SSID, the WiFi Password and the server IP without re-compile the sketch
 How it works:
 1. Connect a button to the pin D6 for ESP6266 or 4 for ESP32
 2. Upload the code
 3. Push the button down for 2-3 seconds. The board will restart as Access Point
 4. Connect your phone to the board wifi network (ESP_Network)
 5. Open the browser, enter the ip: 192.168.4.1
 6. You have to see a web page. Use the web page to configure the SSID, Password and the IP
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

WiFiServer server(8000);                   // Default Virtuino Server port 
char ipAddress[eepromTextVariableSize] ="192.168.1.150";            // where 150 is the desired IP Address. The first three numbers must be the same as the router IP
char gateway[eepromTextVariableSize] ="192.168.1.1";         // set gateway to match your network. Replace with your router IP

   
/* Enable these lines in case you want to change the default Access Point ip: 192.168.4.1. You have to enable the line:   
  WiFi.softAPConfig(local_ip, gateway, subnet); 
  on the void initAsAccessPoint too */
//IPAddress accessPointIP(192,168,1,1);
//IPAddress accesPointGateway(192,168,1,1);
//IPAddress accessPointSubnet(255,255,255,0);


//---VirtuinoCM  Library settings --------------
#include "VirtuinoCM.h"
VirtuinoCM virtuino;               
#define V_memory_count 32          // the size of V memory. You can change it to a number <=255)
float V[V_memory_count];           // This array is synchronized with Virtuino V memory. You can change the type to int, long etc.
//---


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
      //WiFi.softAPConfig(accessPointIP, accesPointGateway, accessPointSubnet);  // enable this line to change the default Access Point IP address
      delay(100);
}

//================================================ initAsClient

void checkWiFiConnection(){
    if (WiFi.status() != WL_CONNECTED) {
        if (lastConnectedStatus==1) Serial.println("WiFi disconnected\n");
        lastConnectedStatus=0;
        Serial.print(".");
        delay(500);
    } else {
      if (lastConnectedStatus==0) {
        Serial.println("Mode= Client"); 
        Serial.print("\nWiFi connectd to :");
        Serial.println(ssid);
        Serial.print("\n\nIP address: ");
        Serial.println(WiFi.localIP());
        
      }
      lastConnectedStatus=1;
    }
}


//================================================ setup
//================================================
void setup() {
  Serial.begin(115200);
  delay(500);

 
   
  //--- Check the first EEPROM byte. If this byte is "2" the board will start as Access Point
  int st=getStatusFromEeprom();
  if (st==2) accessPointMode=true;
  else if (st!=0)  saveSettingsToEEPPROM(ssid,pass,ipAddress,gateway); // run the void saveSettingsToEEPPROM on the first running or every time you want to save the default settings to eeprom
  Serial.println("\n\naccessPointMode="+String(accessPointMode)); 

   readSettingsFromEEPROM(ssid,pass,ipAddress,gateway);    // read the SSID and Passsword from the EEPROM 
 
 if (accessPointMode) {     // start as Access Point
     initAsAccessPoint();
     serverAP.on("/", handle_OnConnect);
     serverAP.onNotFound(handle_NotFound);
     serverAP.begin();
    saveStatusToEeprom(0);  // enable the Client mode for the the next board starting     
  }
    else {            // start as client
      Serial.println("Mode= Client");
       IPAddress subnet(255, 255, 255, 0);        // set subnet mask to match your network
       IPAddress ip_,gateway_;
       ip_.fromString(ipAddress); 
       gateway_.fromString(gateway); 
       Serial.println(ip_.toString());
       Serial.println(gateway_.toString());
   
       WiFi.config(ip_, gateway_, subnet);          // If you don't want to config IP manually disable this line
       WiFi.mode(WIFI_STA);           
       WiFi.begin(ssid, pass);
       server.begin();
      // Enter your client setup code here
    }

   
  pinMode(accessPointButtonPin,INPUT);
  pinMode(accessPointLed,OUTPUT);
  
}

//============================================== loop
//==============================================
void loop() {
  if (accessPointMode) {
    serverAP.handleClient();
    playAccessPointLed();       // blink the LED every time the board works as Access Point
  }
  else {
     checkWiFiConnection();
     virtuinoRun();
   
    // enter your code here
      
    //.....

     
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

//================ Virtuino ======================================



//============================================================== onCommandReceived
//==============================================================
/* This function is called every time Virtuino app sends a request to server to change a Pin value
 * The 'variableType' can be a character like V, T, O  V=Virtual pin  T=Text Pin    O=PWM Pin 
 * The 'variableIndex' is the pin number index of Virtuino app
 * The 'valueAsText' is the value that has sent from the app   */
 void onReceived(char variableType, uint8_t variableIndex, String valueAsText){     
    if (variableType=='V'){
        float value = valueAsText.toFloat();        // convert the value to float. The valueAsText have to be numerical
        if (variableIndex<V_memory_count) V[variableIndex]=value;              // copy the received value to arduino V memory array
    }
}

//==============================================================
/* This function is called every time Virtuino app requests to read a pin value*/
String onRequested(char variableType, uint8_t variableIndex){     
    if (variableType=='V') {
    if (variableIndex<V_memory_count) return  String(V[variableIndex]);   // return the value of the arduino V memory array
  }
  return "";
}


 //==============================================================
  void virtuinoRun(){
   WiFiClient client = server.available();
   if (!client) return;
   if (debug) Serial.println("Connected");
   unsigned long timeout = millis() + 3000;
   while (!client.available() && millis() < timeout) delay(1);
   if (millis() > timeout) {
    Serial.println("timeout");
    client.flush();
    client.stop();
    return;
  }
    virtuino.readBuffer="";    // clear Virtuino input buffer. The inputBuffer stores the incoming characters
      while (client.available()>0) {        
        char c = client.read();         // read the incoming data
        virtuino.readBuffer+=c;         // add the incoming character to Virtuino input buffer
        if (debug) Serial.write(c);
      }
     client.flush();
     if (debug) Serial.println("\nReceived data: "+virtuino.readBuffer);
     String* response= virtuino.getResponse();    // get the text that has to be sent to Virtuino as reply. The library will check the inptuBuffer and it will create the response text
     if (debug) Serial.println("Response : "+*response);
     client.print(*response);
     client.flush();
     delay(10);
     client.stop(); 
    if (debug) Serial.println("Disconnected");
}


 //============================================================== vDelay
  void vDelay(int delayInMillis){long t=millis()+delayInMillis;while (millis()<t) virtuinoRun();}

   
//================ WiFi Manager necessary functions ==============
//================================================================
//================================================================ 
  
//==============================================
void handle_OnConnect() {
  if (debug) Serial.println("Client connected:  args="+String(serverAP.args()));
  if (serverAP.args()==4)  {
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
    else   if (serverAP.argName(i)=="ipAddress") {
      memset(ipAddress, '\0', sizeof(ipAddress));
      strcpy(ipAddress, serverAP.arg(i).c_str());
    }
     else   if (serverAP.argName(i)=="gateway") {
      memset(gateway, '\0', sizeof(gateway));
      strcpy(gateway, serverAP.arg(i).c_str());
    }
  }
  if (debug) Serial.println("*** New settings have received");
  if (debug) Serial.print("*** ssid=");Serial.println(ssid);
  if (debug) Serial.print("*** password=");Serial.println(pass);
  if (debug) Serial.print("*** ipAddress=");Serial.println(ipAddress);
  if (debug) Serial.print("*** gateway=");Serial.println(gateway);
  
  
  saveSettingsToEEPPROM(ssid,pass,ipAddress,gateway);
 
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
  ptr +="<form>";
  ptr +="<div><label for=\"label_1\">WiFi SSID</label><input id=\"ssid_id\" required type=\"text\" name=\"ssid\" value=\"";
  ptr +=ssid;
  ptr +="\" maxlength=\"32\"></div>\n";
  ptr +="<div><label for=\"label_2\">WiFi Password</label><input id=\"pass_id\" type=\"text\" name=\"pass\" value=\"";
  ptr +=pass;
  ptr +="\" maxlength=\"32\"></div>\n";
  

  ptr +="<div><label for=\"label_3\">Server IP</label><input id=\"ipAddress_id\" required type=\"text\" name=\"ipAddress\" value=\"";
  ptr +=ipAddress;
  ptr +="\" maxlength=\"15\" required pattern=\"^([0-9]{1,3}\.){3}[0-9]{1,3}$\"></div>\n";

  ptr +="<div><label for=\"label_4\">Gateway</label><input id=\"gateway_id\" required type=\"text\" name=\"gateway\" value=\"";
  ptr +=gateway;
  ptr +="\" maxlength=\"15\" required pattern=\"^([0-9]{1,3}\.){3}[0-9]{1,3}$\"></div>\n";

  ptr +="<div><input type=\"submit\" value=\"Send\"accesskey=\"s\"></div>";   
  ptr +="</form>";   

  ptr +="<h5>Created by Ilias Lamprou</h5>\n";
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}







//====================== EEPROM necessary functions ==============
//================================================================
//================================================================
#define eepromBufferSize 200     // have to be >  eepromTextVariableSize * (eepromVariables+1)   (33 * (4+1))

//========================================== writeDefaultSettingsToEEPPROM
void saveSettingsToEEPPROM(char* ssid_, char* pass_, char* ipAddress_, char* gateway_){
  if (debug) Serial.println("\n============ saveSettingsToEEPPROM");
  writeEEPROM(1* eepromTextVariableSize , eepromTextVariableSize , ssid_);
  writeEEPROM(2* eepromTextVariableSize , eepromTextVariableSize ,  pass_);
  
  writeEEPROM(3* eepromTextVariableSize , eepromTextVariableSize ,  ipAddress_);
  writeEEPROM(4* eepromTextVariableSize , eepromTextVariableSize ,  gateway_);
 
  
}
//========================================== readSettingsFromEeprom
void readSettingsFromEEPROM(char* ssid_, char* pass_, char* ipAddress_, char* gateway_){
  readEEPROM( 1* eepromTextVariableSize , eepromTextVariableSize , ssid_);
  readEEPROM( (2* eepromTextVariableSize) , eepromTextVariableSize , pass_);
  
  readEEPROM( (3* eepromTextVariableSize) , eepromTextVariableSize , ipAddress_);
  readEEPROM( (4* eepromTextVariableSize) , eepromTextVariableSize , gateway_);
  
  
  if (debug) Serial.println("\n============ readSettingsFromEEPROM");
  if (debug) Serial.print("\n============ ssid=");if (debug) Serial.println(ssid_);
  if (debug) Serial.print("============ password=");if (debug) Serial.println(pass_);
  if (debug) Serial.print("============ ipAddress_=");if (debug) Serial.println(ipAddress_);
  if (debug) Serial.print("============ gateway_=");if (debug) Serial.println(gateway_);
 
  
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
 
