# Tuino 1 Simple Temperature Sensor
This simple IoT demo application delivers a temperature sensor. In this example we are using the LoRaWAN GMX-LR1 module for the Tuino 1.
For a full description of the Tuino 1 check it's GitHub page [TUINO 1](https://github.com/gimasi/TUINO_ONE).<br>


You can use the kit with any LoRaWAN server, or use our Tuino Cloud platform, which uses [Swisscom LPN](http://lpn.swisscom.ch/e/)  or [ThingPark](https://partners.thingpark.com/) by Actility connectivity. 

# Connecting the Sensors/Actuators
The Tuino 1 has 4 Grooove connectors:
* 1 Analog - A0
* 2 Digital - D4 and D5
* 1 I2C

Connect the sensors in the following order to make everything work out of the box

* Temperature on A0
* OLED Display on the I2C port

<br/>
You can connect also the NFC antenna and with an NFC enabled smartphone you can read the temperature read by the sensor.<br/>
<br/>

# Payload Description
Here is a quick description of the payload we have implemented.<br>

<b>Uplink</b>
```C
 temperature_int = current_temperature * 100;

 tx_buf[0] = 0x02; // packet header - multiple data
 tx_buf[1] = (temperature_int & 0xff00 ) >> 8;
 tx_buf[2] = temperature_int & 0x00ff;
```


<br>

# Easter Egg
Even if this is a simple temperature sensor, we have added a small 'easter egg'. Once you have uploaded the script you can go to the Tuino Cloud and send a Text Data to the node.
And here you have your LoRaWAN pager!<br/>
<br/>
<br/>
Have fun and tweak the code and make other great stuff!