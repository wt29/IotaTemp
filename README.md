### IotaWatt 4.8 cloned into IotaTemp 0.1

This code is completely copied from Bob LeMaire's brilliant IotaWatt.

I'm attempting to modify it to provide a simple WeMOS D1 Mini based stand alone temperature node.

IotaWatt has a whole lot of useful stuff including all the goodies below.

See the [WiKi](https://github.com/boblemaire/IoTaWatt/wiki) for detailed information concerning installation and use of IoTaWatt. 

IoTaWatt forum is [here](https://community.iotawatt.com/)

![IotaWatt version 4.8](http://iotawatt.com/Images/IoTaWatt_new_case.JPG)

IotaWatt is an open-hardware/open-source project to produce an accurate, low-cost, multi-channel, and easy to use electric power monitor.  It's based on the ESP8266 IoT platform using MCP3208 12 bit ADCs to sample voltage and current at high sample rates. As a datalogger the device has a real-time clock and SDcard and supports data querry with the integrated web server.

* monitors up to 14 circuits  - Only 1 Channel in IotaTemp
* Accuracy typically within 1%  
* Local SDcard stores up to 15 years worth of data.
* REST APIs to extract data
* Supports many different make, model and capacity current transformers  - not required
* Supports generic definition of any current transformer  - not required
* Three-phase capable  - Sorry Bob, not required
* Local LAN browser based configuration  
* Local LAN browser based status display  
* Local LAN browser based graphing analytic tools  
* Open Hardware/Software 
* Upload to influxDB   
* Upload to [Emoncms.org](https://emoncms.org/)  
* Demo configuration app at [IoTaWatt.com](http://iotawatt.com)  

The software samples input channels at a rate of 35-40 channels per second, recording temperature and humidity to the local SD card every five seconds.  The Emoncms browser based graphing utility can be used on the local WiFi network to view data or the data can be uploaded to an Emoncms server or an influxDB server. In the event of WiFi or internet service disruptions, the IotaWatt will continue to log locally and bulk update the server when WiFi is restored.

The configuration and monitoring app runs on any browser based device that is connected to the local WiFi network. A demo is available at [iotawatt.com](http://iotawatt.com)

Thanks to contributors of other open software that have been incorporated into this project.  In particular the Emoncms, ArduinoJSON and SDWebServer software made a lot of this possible, not to mention the Arduino/ESP8266 project and related forums.

The [WiKi](https://github.com/boblemaire/IoTaWatt/wiki) is a work in progress and provides details of installation and use.
