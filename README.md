# Smart Home Repository
Apps I have made to help automate my house.

# OpenHAB2
I am using OpenHAB2 to help automate my house.

# Current Status
## Smoker Module
The smoker module consists of three parts:
 - The Arduino code
 - The python interface (bluetooth -> MQTT)
 - OpenHAB2 UI
 
The Arduino sends via bluetooth the pit and meat temperature probe temperatures. The Python code (running on the OpenHab2 raspberry pi) connects to the arduino via bluetooth and listens for packets. 

Upon arrival of the packets it decodes and verifies the contents, then forwards it to MQTT. 

Once in MQTT under the topic smoker/pit_temp or smoker/meat_temp it is then picked up by openHAB and displayed in the UI

The python/rpi can automatically reconnect upon disconnection. This is useful because you can run the program on your home automation server, and let it run indefinetly with nohup
```
nohup python smoker_interface_rev1.py &
```

then you can observe the output with 
```
tail -f nohup.out
```
This allows the script to run in the background.

# Contact
Feel free to contact me with any questions regarding my setup and stay tuned for more!

Ian McElhenny - imcelhenny1@gmail.com
