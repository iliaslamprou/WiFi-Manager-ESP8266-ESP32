# WiFi-Manager-ESP8266-ESP32


All the code included in one sketch - no libraries needed<br/><br/>

 Using this code you can configure the WiFi SSID and Password without re-compile the sketch<br/>
 You have to start the board as Access Point using a button and then toy can change the settings from a web page.<br/><br/>
 
 
 
 
 
 ## How it works:<br/><br/>
 
 **1.** Connect a button to the pin D6 for ESP6266 or 4 for ESP32<br/>
 **2.** Upload the code<br/>
 **3.** Push the button down for 2-3 seconds. The board will restart as Access Point<br/>
 **4.** Connect your phone to the board wifi network (ESP_Network)<br/>
 **5.** Open the browser, enter the ip: 192.168.4.1<br/>
 **6.** You have to see a web page. Use the web page to configure the SSID and Password<br/>
 **7.** Push the button again or restart the board. The board will connect to the new network<br/>


 Created by Ilias Lamprou at Jul/10/2020
 
This repository contains tree different examples<br/>

**basic example**  using this example you can change the SSID and Password of the WiFi <br/>
**MQTT example**  using this example you can change the SSID and Password and the MQTT broker url, username and password<br/>
**Virtuino example**  using this example you can change the SSID and Password and the Virtuino Server IP and gateway<br/>


The code created by Ilias Lamprou<br/>
No licence needed<br/>
