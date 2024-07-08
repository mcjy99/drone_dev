# drone_dev_summer_2024
## TODO
- [X] MAVLink over WiFi UDP <-> QGC (test performance with mavros/mavlink-router)
- [X] QGC <-> LoRaWAN gateway over UDP 
- [ ] MAVLink over LoRa serial <-> QGC (on hold, TBD)
- [ ] custom mavlink message
- [ ] build custom QGC
- [ ] TTN webhook -> Thingspeak (if found better use better)
- [ ] power consumption test (idle, sleep mode, active mode, with and without routing etc)
- [ ] help develop physical tests
- [ ] data collection

## Zerotier Setup
### On Windows 
- Sign in to Zerotier. (https://www.zerotier.com/)
- Download Zerotier 1.6.6 for Windows 11. (https://download.zerotier.com/RELEASES/1.6.6/dist/ZeroTier%20One.msi)
- Sign in to Zerotier Central. (https://my.zerotier.com/)
- Create a network and copy the network ID.
- Find the Zerotier One icon by clicking the arrow on the status bar on the window's bottom right.
- Select "Join a Network".
- Paste the network ID in and click "Join"

### On Jetson Terminal (Ubuntu)
- Run the ubuntuzerotiersetup.sh by changing the 9e1948db6317a4d2 to network ID in the Zerotier website network created.

### Connection
- Disconnected all wifi and open a command prompt and type "ipconfig" for Windows or "ifconfig" for Ubuntu/Linux.  
- Run the start-mav-route.sh by changing the IP address to the QGC device IP address without using Wifi.

## Glossary
- QGC-QGroundControl

## Links
### PX4 development setup
https://docs.px4.io/main/en/dev_setup/dev_env_windows_wsl.html  
https://docs.px4.io/main/en/dev_setup/building_px4.html  

### MAVLink 
https://mavlink.io/en/guide/serialization.html   

### Custom MAVLink
https://docs.px4.io/main/en/middleware/mavlink.html  
https://gist.github.com/vo/84dc2daf9f51d7901d26fd8e1efb47ad  
https://mavlink.io/en/getting_started/generate_libraries.html  
https://docs.px4.io/main/en/ros/mavros_custom_messages.html  

### TTN
https://www.thethingsindustries.com/docs/integrations/  
https://www.thethingsindustries.com/docs/integrations/cloud-integrations/  
https://www.thethingsindustries.com/docs/integrations/cloud-integrations/thingspeak/  
