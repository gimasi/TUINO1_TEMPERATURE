/* _____  _____  __  __             _____  _____
  / ____||_   _||  \/  |    /\     / ____||_   _|
  | |  __   | |  | \  / |   /  \   | (___    | |
  | | |_ |  | |  | |\/| |  / /\ \   \___ \   | |
  | |__| | _| |_ | |  | | / ____ \  ____) | _| |_
  \_____||_____||_|  |_|/_/    \_\|_____/ |_____|
  (c) 2017 GIMASI SA

   tuino1_bootcamp.ino

    Created on: May 5, 2017
        Author: Massimo Santoli massimo@gimasi.ch
        Brief: Tuino1 Maker's Kit IoT Temperature Demo
        Version: 1.0

        License: it's free - do whatever you want! ( provided you leave the credits)

*/

#include "gmx_lr.h"
#include "SeeedOLED.h"
#include "display_utils.h"
#include <Wire.h>

//value for the uplink period timer
long int timer_period_to_tx = 60000;
long int timer_millis_lora_tx = 0;
int ledState = 0;

//The pins where to connect your external hardware
int buttonPin = D4;
int tempSensorPin = A0;

String oled_string;

// Temperature Sensor constants, use these to calibrate
const int B = 4275;             // B value of the thermistor
const int R0 = 100000;          // R0 = 100k

#define TEMPERATURE_SAMPLING    (20)
#define THERMOSTAT_HYSTERESIS   (1.0f)

float current_temperature = 0.0f;
float temp_temperature = 0;
int temperature_counts = 0;

bool data_received = false;
//callback function on received data
void loraRx() {
  //setting a flag that can be read in the main loop
  data_received = true;
}

//temperature reading and converting function
float readTemp() {
  int a = analogRead(tempSensorPin );
  float R = 1023.0 / ((float)a) - 1.0;
  R = 100000.0 * R;
  
  //convert to temperature via datasheet
  float temperature = 1.0 / (log(R / 100000.0) / B + 1 / 298.15) - 273.15; 
  return temperature;
}

//function against button debouncing
void waitButton() {
  while ( digitalRead(buttonPin) == 0 ){  };
  delay(200);
}

//Setup the radio and join the network
void setup() {
  String DevEui,NewDevEui;
  String AppEui,NewAppEui;
  String AppKey,NewAppKey;
  String loraClass,LoRaWANClass;

  char string[64];

  String adr, dcs, dxrate;

  byte join_status;
  int join_wait;

  Wire.begin();
  Serial.begin(9600);
  Serial.println("Starting");

  // Init Oled
  SeeedOled.init();  //initialze SEEED OLED display

  SeeedOled.clearDisplay();          //clear the screen and set start position to top left corner
  SeeedOled.setNormalDisplay();      //Set display to normal mode (i.e non-inverse mode)
  SeeedOled.setHorizontalMode();           //Set addressing mode to Page Mode

  // GMX-LR init pass callback function
  gmxLR_init(&loraRx);

  //Set the LoRaWAN class (available: A and C)
  LoRaWANClass = "C";
  gmxLR_setClass(LoRaWANClass);

  // Disable Duty Cycle  ONLY FOR DEBUG!
  gmxLR_setDutyCycle("0");
  
  // Show Splash Screen on OLED
  splashScreen();
  delay(2000);

  // Show LoRaWAN Params on OLED
  gmxLR_getDevEui(DevEui);
  gmxLR_getAppKey(AppKey);
  gmxLR_getAppEui(AppEui);
  displayLoraWanParams(DevEui, AppEui, AppKey);

  delay(2000);
  
  SeeedOled.clearDisplay();
  SeeedOled.setTextXY(0, 0);
  SeeedOled.putString("Joining...");

  Serial.println("Joining...");
  join_wait = 0;
  while ((join_status = gmxLR_isNetworkJoined()) != LORA_NETWORK_JOINED) {
    
    if ( join_wait == 0 )
    {
      Serial.println("LoRaWAN Params:");
      gmxLR_getDevEui(DevEui);
      Serial.println("DevEui:" + DevEui);
      gmxLR_getAppEui(AppEui);
      Serial.println("AppEui:" + AppEui);
      gmxLR_getAppKey(AppKey);
      Serial.println("AppKey:" + AppKey);
      gmxLR_getClass(loraClass);
      Serial.println("Class:" + loraClass);
      adr = String( gmxLR_getADR() );
      Serial.println("ADR:" + adr);
      dcs = String( gmxLR_getDutyCycle() );
      Serial.println("DCS:" + dcs);
      gmxLR_getRX2DataRate(dxrate);
      Serial.println("RX2 DataRate:" + dxrate);
      gmxLR_Join();
    }

    SeeedOled.setTextXY(1, 0);
    sprintf(string, "Attempt: %d", join_wait);
    SeeedOled.putString(string);

    join_wait++;

    if (!( join_wait % 100 )) {
      gmxLR_Reset();
      join_wait = 0;
    }

    delay(5000);
  };

  SeeedOled.setTextXY(2, 0);
  SeeedOled.putString("Joined!");

  delay(2000);
  SeeedOled.clearDisplay();

  // Init Temperature
  current_temperature = readTemp();
  oled_string = "                                                  ";
}

//This is the main code and will constantly run in a loop
void loop() {
  long int time_to_tx;
  int temperature_int;
  char lora_data[128];
  char lora_rx_string[256];
  byte tx_buf[128];
  byte rx_buf[128];
  int buf_len;

  String rx_data;
  String tx_data;
  String lora_string;
  int port;

  //Update the timer
  time_to_tx = millis() - timer_millis_lora_tx;

  // Transmit at timer overflow
  if (( time_to_tx > timer_period_to_tx) || (timer_millis_lora_tx == 0 )) {

    //multiply the temperature with 100 to avoid sending as float value
    temperature_int = current_temperature * 100;

    Serial.println(temperature_int);

    //define payload: packet header and temperature on 2 bytes
    tx_buf[0] = 0x02; // packet header - multiple data
    tx_buf[1] = (temperature_int & 0xff00 ) >> 8;
    tx_buf[2] = temperature_int & 0x00ff;

    //write payload data into a String type accepted by module
    sprintf(lora_data, "%02X%02X%02X", tx_buf[0], tx_buf[1], tx_buf[2] );

    displayLoraTX(true);
    Serial.println(lora_data);
    tx_data = String(lora_data);
    Serial.println("TX DATA:" + tx_data);
    //actual send function on LoRaWAN:
    gmxLR_TXData(tx_data);

    //reset timer
    timer_millis_lora_tx = millis();
    displayLoraTX(false);
  }
  else
  {
    displayTime2TX(timer_period_to_tx - time_to_tx);
  }
  // End Trasmission

  if (data_received)
  {
    displayLoraRX(true);

    gmxLR_RXData(rx_data, &port);
    gmxLR_StringToHex(rx_data, rx_buf, &buf_len );

    Serial.println("LORA RX DATA:" + rx_data);
    Serial.print("LORA RX LEN:");
    Serial.println(buf_len);
    Serial.print("LORA RX PORT:");
    Serial.println(port);

    //display received data on OLED screen
    rx_buf[buf_len] = 0;
    sprintf(lora_rx_string,"%s",rx_buf);
    Serial.println( lora_rx_string );
    oled_string = String( lora_rx_string );

    data_received = false;

    delay(1000);
    displayLoraRX(false);
  }

  displayTemp(current_temperature,oled_string );

  // update temperature
  // we sample and make an average of the temperature - since in this demo we use a NTC sensor that fluctuates
  temp_temperature += readTemp();
  temperature_counts ++;

  if ( temperature_counts >= TEMPERATURE_SAMPLING )
  {
    current_temperature = temp_temperature / TEMPERATURE_SAMPLING;

    temperature_counts = 0;
    temp_temperature = 0;
  }
  
}
