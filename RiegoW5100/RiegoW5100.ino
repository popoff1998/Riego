/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik EKblad
 * Contribution by a-lurker and Anticimex,
 * Contribution by Norbert Truchsess <norbert.truchsess@t-online.de>
 * Contribution by Tomas Hozza <thozza@gmail.com>
 *
 *
 * DESCRIPTION
 * The EthernetGateway sends data received from sensors to the ethernet link.
 * The gateway also accepts input on ethernet interface, which is then sent out to the radio network.
 *
 * The GW code is designed for Arduino 328p / 16MHz.  ATmega168 does not have enough memory to run this program.
 *
 * LED purposes:
 * - To use the feature, uncomment WITH_LEDS_BLINKING in MyConfig.h
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error
 *
 * See http://www.mysensors.org/build/ethernet_gateway for wiring instructions.
 *
 */

// Enable debug prints to serial monitor
//#define MY_DEBUG 

// Enable and select radio type attached
//#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Enable gateway ethernet module type 
#define MY_GATEWAY_W5100

// W5100 Ethernet module SPI enable (optional if using a shield/module that manages SPI_EN signal)
//#define MY_W5100_SPI_EN 4  

// Enable Soft SPI for NRF radio (note different radio wiring is required)
// The W5100 ethernet module seems to have a hard time co-operate with 
// radio on the same spi bus.
#if !defined(MY_W5100_SPI_EN) && !defined(ARDUINO_ARCH_SAMD)
  #define MY_SOFTSPI
  #define MY_SOFT_SPI_SCK_PIN 14
  #define MY_SOFT_SPI_MISO_PIN 16
  #define MY_SOFT_SPI_MOSI_PIN 15
#endif  

// When W5100 is connected we have to move CE/CSN pins for NRF radio
#ifndef MY_RF24_CE_PIN 
  #define MY_RF24_CE_PIN 5
#endif
#ifndef MY_RF24_CS_PIN 
  #define MY_RF24_CS_PIN 6
#endif

// Enable to UDP          
//#define MY_USE_UDP

#define MY_IP_ADDRESS 192,168,100,61   // If this is disabled, DHCP is used to retrieve address
// Renewal period if using DHCP
//#define MY_IP_RENEWAL_INTERVAL 60000
// The port to keep open on node server mode / or port to contact in client mode
#define MY_PORT 5003      

// Controller ip address. Enables client mode (default is "server" mode). 
// Also enable this if MY_USE_UDP is used and you want sensor data sent somewhere. 
//#define MY_CONTROLLER_IP_ADDRESS 192, 168, 100, 60  
 
// The MAC address can be anything you want but should be unique on your network.
// Newer boards have a MAC address printed on the underside of the PCB, which you can (optionally) use.
// Note that most of the Ardunio examples use  "DEAD BEEF FEED" for the MAC address.
#define MY_MAC_ADDRESS 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEC

// Flash leds on rx/tx/err
#define MY_LEDS_BLINKING_FEATURE
// Set blinking period
#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
#define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60 
// Digital pin used for inclusion mode button
#define MY_INCLUSION_MODE_BUTTON_PIN  3 

// Uncomment to override default HW configurations
//#define MY_DEFAULT_ERR_LED_PIN 7  // Error led pin
//#define MY_DEFAULT_RX_LED_PIN  8  // Receive led pin
//#define MY_DEFAULT_TX_LED_PIN  9  // the PCB, on board LED

//Defines de configuracion
#define MY_NODE_ID 5

#include <SPI.h>

#if defined(MY_USE_UDP)
  #include <EthernetUdp.h>
#endif
#include <Ethernet.h>
#include <MySensors.h>

//Estructura de reles
typedef struct  {
  int  pin;
  char desc[20];
  bool ON;
  bool OFF;
} sRELE;

sRELE Rele [] = { {13 , "LED", HIGH, LOW},
                  {31 , "RELE GOTEROS ALTOS", LOW, HIGH},
                  {33 , "RELE ASPERSOR FONDO", LOW, HIGH},
                  {35 , "LUZ DEL PORCHE", LOW, HIGH}
                };

#define NUMBER_OF_RELAYS 4 // Total number of attached relays


void setup()
{
  //Para los reles
    Serial.println("start call SETUP");
  for (int sensor=0 ; sensor<NUMBER_OF_RELAYS;sensor++) {
    // Poner el rele en output mode
    pinMode(Rele[sensor].pin, OUTPUT);   
    // Poner el rele en el ultimo estado conocido (usando eeprom storage) 
    digitalWrite(Rele[sensor].pin, loadState(sensor)?Rele[sensor].ON:Rele[sensor].OFF);
  }
    Serial.println("End call Setup");
}

void presentation()
{
  // Presentar los sensores y actuadores locales 
  Serial.println("start call presentation");
  // Mandar la info del sketch
  sendSketchInfo("Arduino ETH", "1.0");
  //Presentar los reles
  for (int rele=0; rele<NUMBER_OF_RELAYS;rele++) {
    // Registrar todos los reles al gw
    present(rele, S_LIGHT,Rele[rele].desc);
  }
  Serial.println("End call presentation");
}

void loop() {
  sendBatteryLevel(50);
  wait(2000);
}

void receive(const MyMessage &message) {
  // Solo esperamos mensajes V_LIGTH de momento, pero lo chequeamos por si acaso.
  if (message.type==V_LIGHT) {
     // Cambiar estado del rele
     digitalWrite(Rele[message.sensor].pin, message.getBool()?Rele[message.sensor].ON:Rele[message.sensor].OFF);
     // Almacenar estado en la eeprom
     saveState(message.sensor, message.getBool());
     // Escribir informacion de debug
     Serial.print("Cambio entrante para sensor:");
     Serial.print(message.sensor);
     Serial.print(", Nuevo status: ");
     Serial.println(message.getBool());
   } 
}

