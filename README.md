# Modern Green Thumb Tracker

This is the code for an Arduino based Wemos D1 mini board with an ESP8266 wireless chip onboard, a capacitive moisture sensor and battery pack. 
It is used as a soil moisture sensor and can be programmed using the accompanying [Modern Green Thumb Application.](https://github.com/JuanCPDev/ModernGreenThumbApp)

Uses:

The device starts by creating its own access point and creating its own asynchronous web server using HTTP protocols, which then the user can connect to. 
After using the application, the user would be able to add the device to their account and connect to their local Wi-Fi network. The application does this by sending HTTP
POST commands to the server, which the device then verifies the information and after a successful connection, saves it to the onboard EEPROM. The device then is able to 
turn on its moisture sensor every 24 hours, take the average reading and send it to the [Modern Green Thumb API](https://github.com/JuanCPDev/ModernGreenThumbServer) using HTTP
PUT commands, which in turns takes the information and updates the database with the values. The device uses the ESP8266 deep sleep power mode, which uses less than ~20ua  
of power in between readings, to prolong the onboard battery.
