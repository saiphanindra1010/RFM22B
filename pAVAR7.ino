
 #include <avr/io.h>
#include <avr/interrupt.h>
#include <util/crc16.h>
#include <SPI.h>
#include <RFM22.h>

#define CALLSIGN "NULL"
#define ASCII 7          // ASCII 7 or 8
#define STOPBITS 2       // Either 1 or 2
#define TXDELAY 0        // Delay between sentence TX's
#define RTTY_BAUD 50     // Baud rate for use with RFM22B Max = 600
#define RADIO_FREQUENCY 434.575 // Actually frequency can be +/- 0.005Mhz 
#define RADIO_REBOOT 20  // Reboot Radio every X telemetry lines
#define ONE_SECOND 245
#define RADIO_POWER  0x04
/*
 0x02  5db (3mW)
 0x03  8db (6mW)
 0x04 11db (12mW) // UK Legal Limit
 0x05 14db (25mW)
 0x06 17db (50mW)
 0x07 20db (100mW)
 */

#define RFM22B_PIN 10
#define RFM22B_SDN 3
#define STATUS_LED 4            // PAVA R7 Boards have an LED on PIN4
#define BATTERY_ADC  A0

#define POWERSAVING      // Comment out to turn power saving off

uint8_t buf[60]; 
char txstring[80];
volatile int txstatus=1;
volatile int txstringlength=0;
volatile char txc;
volatile int txi;
volatile int txj;
volatile int count=1;
volatile boolean lockvariables = 0;
uint8_t lock =0, sats = 0, hour = 0, minute = 0, second = 0;
uint8_t oldhour = 0, oldminute = 0, oldsecond = 0;
int navmode = 0, GPSerror = 0, lat_int=0,lon_int=0,txnulls=10;
int32_t lat = 0, lon = 0, alt = 0, maxalt = 0, lat_dec = 0, lon_dec =0;
int psm_status = 0, radiostatus=0, countreset=0;
unsigned long currentMillis,previousMillis=0;
int32_t tslf=0;
int errorstatus=0; 


rfm22 radio1(RFM22B_PIN);

void setup() {
  pinMode(STATUS_LED, OUTPUT); 
  blinkled(6);
  Serial.begin(9600);
  blinkled(5);
  resetGPS();
  blinkled(4);
  wait(500);
  blinkled(3);
  setupRadio();
  blinkled(2);
  setupGPS();
  blinkled(1);
  initialise_interrupt();
#ifdef POWERSAVING
  ADCSRA = 0;
#endif
}/* Bit 0 = GPS Error Condition Noted Switch to Max Performance Mode
 Bit 1 = GPS Error Condition Noted Cold Boot GPS
 Bit 2 = RFM22B Error Condition Noted, RFM22B Power Cycled
 Bit 3 = Current Dynamic Model 0 = Flight 1 = Pedestrian
 Bit 4 = PSM Status 0 = PSM On 1 = PSM Off                   
 Bit 5 = Lock 0 = GPS Locked 1= Not Locked
 */

void loop()
{
  oldhour=hour;
  oldminute=minute;
  oldsecond=second;
  gps_check_nav();

  if(lock!=3) 
  {
    errorstatus |=(1 << 5);     
  }
  else
  {
    errorstatus &= ~(1 << 5);
  }
  checkDynamicModel();
