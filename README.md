Wifi Power Strip
================

Software:
---------

C++ source, HTML, Javascript & JSON communication with the web interface.

* Full MQTT: set, state and schema allowing auto-detection in home-assistant,
* Allows to control up to 6 switches per ESP8266 module,
* Also allows controlling multiple outputs with a single defined input (for power strips),
* Can control up to 12 switches with an additional slave module connected via the serial port,
* allows timers on each output,
* possibility of disabling the output timer by holding its corresponding switch for a few seconds (or when ordering its ignition when only one switch is available - ex. tuya power strip),
* several configurables SSID,
* configurable reversibility on each output (allowing different interface cards),
* web interface configuration, status and set,
* available storage of the output states on reboot,
* firmware update via WiFi (OTA), without loss of setting,
* can control contact sensors (via the definition of virtual outputs),
* the html interface can be delocalized to a dedicated server,
* dedug trace available by telnet console (only in the C beta-version),
* each object (wifi manager, pins manager, mqtt or ntp manager) is reusable for other projects...

Http requests available:
<table>
  <tr>
    <td><tt>http://IPAddress</tt></td>
    <td>WEB User Interface</td>
  </tr>
  <tr>
    <td><tt>http://IPAddress/status</tt></td>
    <td>get the device state in JSON format - also used by the html interface</td>
  </tr>
  <tr>
    <td><tt>http://IPAddress/status?{'JSON setting'}</tt></td>
    <td>set all or parts of the state setting (see above)</td>
  </tr>
  <tr>
    <td><tt>http://IPAddress/restart</tt></td>
    <td>reboot the device, the output states are preserved</td>
  </tr>
</table>

TODO:
* set the "Access-Control-Allow-Origin" parameter (to allow a domain name use),
* test the new serial communication with the slave,
* add the trigger setting programmed in the interface,
* finalize the setting of all the mqtt parameters in the interface (default values in setting.h),
* do the dev doc,
* ...

Screenshots:

![](doc/images/screenshot.png)

![](doc/images/about.png)

MQTT parameters:

![](doc/images/MQTT-Screenshot.png)

Hardware:
---------

* Webmos D1 mini + wiring diagram of the interfaces (with 2N7002 or S8050):
* ![](doc/images/schema.png)
* Progammable contactors with WiFi control: ![](doc/images/programmableContactor.jpg) ![](doc/images/contactor-relay.png)
* Interface board: 1 x Wemos mini D1, 15 x 1N5819, 3 x Zeners 3.3v, 6 resistors 360 ohms, 6 x MOSFET 2N7002 SOT23 and somes connectors... for each master / slave module (via UART) of 6 inputs and 6 outputs for power interface control (and SSR relay interface or, best to turn off led lights: 5V relay module board, 6 channels).
* ![](doc/images/contactor-MS.jpg)
* With 3D printed case: ![](doc/images/programmableContactorWith3DCase.jpg)
* Master/Slave modules: ![](doc/images/master-slave.jpg)
* switchboard (5-way module for lighting control): ![](doc/images/switchboard.jpg)
* Example of selector definition in Domoticz: ![](doc/images/domoticz-selector.png)
* Plan view ("Fixe-the-State" to inhibit the internal timer, such a 3-second press on switch):
* ![](doc/images/domoticz-selector-view.png)
* ![](doc/images/3d-print.jpg)

* relay modules (Serial Master/Slave):
* ![](doc/images/contact-box.png)

* USB WiFi-relay (same code with ESP01-1m):
* ![](doc/images/USB_WiFi-relay.jpg)

* Hacking - same code on TUYA power strip (TYWE3S):
* ![](doc/images/tuya1.jpg)
* ![](doc/images/tuya2.jpg)
