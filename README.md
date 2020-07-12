# WiFi-Manager-ESP8266-ESP32




 Using this code you can configure the WiFi SSID and Password without re-compile the sketch<br/>
 You have to start the board as Access Point using a button. Then you can change the settings using a web page.<br/><br/>
 
 All the code is included in one sketch - no libraries needed<br/><br/>

 ![wifi_namager_ip_gw](https://user-images.githubusercontent.com/24625307/87254439-542db000-c48b-11ea-9aec-0514be0f9281.png) 
 
 ## How it works:<br/><br/>
 
 **1.** Connect a button to the pin D6 for ESP6266 or 4 for ESP32<br/>
 **2.** Upload the code<br/>
 **3.** Push the button down for 2-3 seconds. The board will restart as Access Point<br/>
 **4.** Connect your phone or laptop to the board wifi network (ESP_Network)<br/>
 **5.** Open the browser, enter the ip: 192.168.4.1<br/>
 **6.** You have to see a web page. Use the web page to configure the SSID and Password<br/>
 **7.** Push the button again or restart the board. The board will connect to the new network<br/>

  
This repository contains tree different examples<br/>

**basic example:**  using this example you can change the SSID and Password of the WiFi <br/>
**MQTT example:**  using this example you can change the SSID and Password and the MQTT broker url, username and password<br/>
**Virtuino example:**  using this example you can change the SSID and Password and the Virtuino Server IP and gateway<br/>


The code is created by Ilias Lamprou<br/>
No licence needed<br/><br/><br/>

Jul-12-2020<br/>
