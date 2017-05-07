/* _____  _____  __  __             _____  _____
  / ____||_   _||  \/  |    /\     / ____||_   _|
  | |  __   | |  | \  / |   /  \   | (___    | |
  | | |_ |  | |  | |\/| |  / /\ \   \___ \   | |
  | |__| | _| |_ | |  | | / ____ \  ____) | _| |_
  \_____||_____||_|  |_|/_/    \_\|_____/ |_____|
  (c) 2017 GIMASI SA

   tuino1_temperature.ino

    Created on: May 5, 2017
        Author: Massimo Santoli massimo@gimasi.ch
        Brief: Tuino1 Maker's Kit IoT Temperature Demo
        Version: 1.0

        License: it's free - do whatever you want! ( provided you leave the credits)

*/

#include "gmx_lr.h"
#include "Regexp.h"
#include "SeeedOLED.h"
#include "display_utils.h"

#include <Wire.h>

// NFC
#include "M24SR.h"
#include "NdefMessage.h"
#include "NdefRecord.h"


// RegExp Engine for NFC parsing
MatchState nfc_ms;
char regexp_buf[512];

#define gpo_pin TUINO_NFC_INTERRUPT

bool writing_nfc = false;

M24SR m24sr04(gpo_pin);
// END NFC

long int timer_period_to_tx = 60000;
long int timer_millis_lora_tx = 0;
int ledState = 0;

int buttonPin = D4;
int relayPin = D5;
int tempSensorPin = A0;

String oled_string;

// Temperature Sensor constants
const int B = 4275;               // B value of the thermistor
const int R0 = 100000;          // R0 = 100k

#define TEMPERATURE_SAMPLING    (20)
#define THERMOSTAT_HYSTERESIS   (1.0f)

float current_temperature = 0.0f;
float temp_temperature = 0;
int temperature_counts = 0;


// LoRa RX interrupt

bool data_received = false;

void loraRx() {
  data_received = true;
}


float readTemp() {
  int a = analogRead(tempSensorPin );

  float R = 1023.0 / ((float)a) - 1.0;
  R = 100000.0 * R;

  float temperature = 1.0 / (log(R / 100000.0) / B + 1 / 298.15) - 273.15; //convert to temperature via datasheet ;

  return temperature;
}



void writeNFC() {
  char string[32];
  String tmpString;
  String nfc_data;

  gmxLR_getDevEui(tmpString);
  nfc_data = "DevEUI:" + tmpString;
  nfc_data = nfc_data + "\n\r";

  nfc_data = nfc_data + "Temp:" + String(readTemp());
  nfc_data = nfc_data + "\n\r";
  nfc_data = nfc_data + "Text:" + oled_string;
  nfc_data = nfc_data + "\n\r";

  // WriteNFC data
  writing_nfc = true;
  NdefMessage message = NdefMessage();
  message.addTextRecord(nfc_data);
  m24sr04.writeNdefMessage(&message);
  delay(200);
  writing_nfc = false;
}


void waitButton() {

  while ( digitalRead(buttonPin) == 0 )
  {

  };

  delay(200);
}


void setup() {
  // put your setup code here, to run once:
  String DevEui;
  String AppEui;
  String AppKey;
  String loraClass;

  char string[64];

  String adr, dcs, dxrate;

  byte join_status;
  int join_wait;

  Wire.begin();
  Serial.begin(9600);
  Serial.println("Starting");

  // Startupp NFC
  m24sr04._setup();

  // Init Oled
  SeeedOled.init();  //initialze SEEED OLED display

  SeeedOled.clearDisplay();          //clear the screen and set start position to top left corner
  SeeedOled.setNormalDisplay();      //Set display to normal mode (i.e non-inverse mode)
  SeeedOled.setHorizontalMode();           //Set addressing mode to Page Mode


  // GMX-LR init pass callback function
  gmxLR_init(&loraRx);

  // Set AppEui and AppKey
  // Uncomment these if you want to change the default keys
  // AppEui = "00:00:00:00:00:00:00:00";
  // AppKey = "6d:41:46:39:67:4e:30:56:46:4a:62:4c:67:30:58:33";


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
      // If AppKey and/or AppEUI are specified set them
      if (AppEui.length() > 0 )
        gmxLR_setAppEui(AppEui);
      if (AppKey.length() > 0 )
        gmxLR_setAppKey(AppKey);

      // Disable Duty Cycle  ONLY FOR DEBUG!
      gmxLR_setDutyCycle("0");

      // Set LoRaWAN Class
      gmxLR_setClass("C");

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

  writeNFC();
  delay(2000);
  SeeedOled.clearDisplay();

  // Init Temperature
  current_temperature = readTemp();
  oled_string = "                                                  ";

}

void loop() {
  // put your main code here, to run repeatedly:

  long int delta_lora_tx;
  int temperature_int;
  char lora_data[32];
  char lora_rx_string[256];
  byte tx_buf[32];
  byte rx_buf[256];
  int buf_len;

  String rx_data;
  String tx_data;
  String lora_string;
  int port;

  delta_lora_tx = millis() - timer_millis_lora_tx;

  // Transmit Period
  if (( delta_lora_tx > timer_period_to_tx) || (timer_millis_lora_tx == 0 )) {

    temperature_int = current_temperature * 100;

    Serial.println(temperature_int);

    tx_buf[0] = 0x02; // packet header - multiple data
    tx_buf[1] = (temperature_int & 0xff00 ) >> 8;
    tx_buf[2] = temperature_int & 0x00ff;
  
    sprintf(lora_data, "%02X%02X%02X", tx_buf[0], tx_buf[1], tx_buf[2] );

    displayLoraTX(true);
    Serial.println(lora_data);
    tx_data = String(lora_data);
    Serial.println("TX DATA:" + tx_data);
    gmxLR_TXData(tx_data);


    timer_millis_lora_tx = millis();


    // Update NFC
    writeNFC();

    displayLoraTX(false);
  }
  else
  {
    displayTime2TX(timer_period_to_tx - delta_lora_tx);
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
