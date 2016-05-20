// Arduino Shields LED/RTC Shield demo code
// www.arduino.com.au
// v1.01 13-Oct-2011 - attach a 2ms interrupt for LED driver
// tested on Arduino Uno

// SimulateLED(10,10,400,300,0x0000ff);

#include <TimerOne.h> // include Timer1 routines for 2ms timer oveflow interrupt
#include <SPI.h>   // inslude the SPI library:
#include "sevenseg.h" // include 7segment display char defintions
#include "DS1302.h" // include Dallas DS1302 clock chip driver
byte rtc[7];

const int BeeperPin = 9;      // piezo beeper pin
const int CsPin = 8;      // Chip select pin to load display registers
const int cc[4] = {8,4,2,1}; // common cathode drive sequence

const byte RSTPin = 5; // DS1302 reset pin
const byte IOPin = 6;  // DS1302 input/output pin
const byte CLKPin = 7;// DS1302 clock pin


const int switch1 = 0; // left switch
const int switch2 = 1;
const int switch3 = 2;
const int switch4 = 3; // right switch

byte serialin,col,ledmem[4],secs; // various variables
int scol = 3; // must be int so can go negative
unsigned long ms; // note time needs to be unsigned longs
  
void setup()
{
  Serial.begin(115200);        // initialize the serial communication:
  pinMode(BeeperPin, OUTPUT);   // initialize Beeper output
  pinMode(CsPin, OUTPUT);   // initialize CS output
  pinMode(RSTPin, OUTPUT);   // initialize DS1302 reset output
  pinMode(IOPin, OUTPUT);   // initialize DS1302 IO output
  pinMode(CLKPin, OUTPUT);   // initialize DS1302 Clk output
  SPI.begin();       // initialize SPI:
  col = 0;

  Timer1.initialize(2000);         // initialize timer1, and set a 1/2 second period
  Timer1.attachInterrupt(timer_2ms);  // attaches callback() as 
 
  secs = 12; // ensure clock doesn't override
  ledmem[3] = 0xff;
  ledmem[2] = 0xff;
  ledmem[1] = 0xff;
  ledmem[0] = 0xff; // display -Hi- for 3 seconds
  delay(1000);
 
  
  secs = 12;
  ledmem[3] = L_bar;
  ledmem[2] = L_H;
  ledmem[1] = L_I;
  ledmem[0] = L_bar; // display -Hi- for 3 seconds
}


void  new_second(void) // here once every second
{  
  if (secs>0) { // countdown secs to 0
    secs--;
    if (secs==0) {
      secs = 10; // then reset to 10
      scol = 3; // reset serial char ptr to left 7seg display
    }
  }
  read_clock();    // read the clock once a second
  serial_clock();  // may as well send this out the serial port
}

void timer_2ms() // 2ms timer1 overflow interrupt
{
    ms++; // 2ms counter
    if ((ms%500)==0) { // setup new second routine
      new_second(); 
    }
   
     if (secs <3) { // display S seconds for 3 seconds
      ledmem[3] = L_S;
      ledmem[2] = 0;
      show(1,rtc[DS_SEC]);
      ledmem[2] = ledmem[2] | 0x80; // add in dp       
    } else if (secs<10) { // for 7 seconds display hh.mm
      show(3,rtc[DS_HOUR]);
      show(1,rtc[DS_MIN]);
      ledmem[2] = ledmem[2] | 0x80; // show dp by default
      if ((ms&0x80)==0) { // flash dp every 256ms
        ledmem[2] = ledmem[2] & 0x7f;
      }   
    }   
    
    SPI.transfer(cc[col]);  // send out com,mon cathode driver via uln2003a
    SPI.transfer(ledmem[col]); // send out anode display data
    digitalWrite(CsPin,1); // latch in 595 registers
    digitalWrite(CsPin,0);
    col++; // increment display counter to next column to the right
    if (col>3) {
      col = 0;
    }
}
 

void loop() 
{  
  if (Serial.available()) {   // check if data has been sent from the computer:
    secs = 12; // turn off clock display for 2 seconds
    serialin = Serial.read(); // read in serial data
    ledmem[scol] = ascii7[serialin];  // show on display
    scol--; // move to next display
    if (scol<0) {
      scol = 3;
    }
  }  

  if (analogRead(switch1)<10) { // if sw1 pressed
    clearleds();  // clear display and stop clock for 2 seconds
    noInterrupts(); // stop 1sec clock read
    setclk(); // set clock to 19:58 20/09/11
    interrupts(); // re-enable
    ledmem[3] = L_S; // flash SET1 while sw1 pressed
    ledmem[2] = L_E;
    ledmem[1] = L_T;
    ledmem[0] = L_1;
    tone(BeeperPin,1000,20); // beep at 1kHz for 20ms
  }

  if (analogRead(switch2)<10) {    // read the input pin
    clearleds(); // clear display and stop clock for 2 seconds
    ledmem[1] = L_2; // display _2__
    tone(BeeperPin,2000,20); // beep at 2kHz for 20ms  
  }

  if (analogRead(switch3)<10) {    // read the input pin
    clearleds(); // clear display and stop clock for 2 seconds
    ledmem[2] = L_3;// display __3_
    tone(BeeperPin,3000,20); // beep at 3kHz for 20ms
  }

  if (analogRead(switch4)<10) {    // read the input pin
    clearleds(); // clear display and stop clock for 2 seconds
    ledmem[3] = L_4; // display ___4
    tone(BeeperPin,4000,20); // beep at 4kHz for 20ms    
  }
}

void  clearleds(void)
{
    secs = 12; // stop clock update for 2 seconds
    ledmem[3] = 0; // clear all display characters
    ledmem[2] = 0;    
    ledmem[1] = 0;
    ledmem[0] = 0;    
}


void  outdata(byte b) // send out a 2digit bcd number
{
  Serial.print(b>>4);
  Serial.print(b&0x0f);
}

void serial_clock(void) // send out clock via serial port
{
  outdata(rtc[DS_HOUR]);
  Serial.write(':');
  outdata(rtc[DS_MIN]);
  Serial.write(':');
  outdata(rtc[DS_SEC]);
  Serial.write(' ');
  outdata(rtc[DS_DAY]);
  Serial.write('/');
  outdata(rtc[DS_MON]);
  Serial.write('/');
  Serial.write('2');
  Serial.write('0');
  outdata(rtc[DS_YEAR]);
  Serial.write(0x0d);
  Serial.write(0x0a);
}


byte    get_ds1302(byte b) // read ds1302 byte at address b
{
byte i;

  digitalWrite(CLKPin,0);
  digitalWrite(RSTPin,1);
  pinMode(IOPin,OUTPUT);
  
  b = b<<1;
  b = b | 0x81;

  for (i=0; i<8; i++) {
   digitalWrite(CLKPin,0);    
//   delayMicroseconds(Clk_delay);   
   if (b&0x01) {
    digitalWrite(IOPin,1);     
   } else {
    digitalWrite(IOPin,0);     
   }
   b = b>>1;
   digitalWrite(CLKPin,1);   
 //  delayMicroseconds(Clk_delay);      
  }

   pinMode(IOPin, INPUT);   // initialize Beeper output
   digitalWrite(CLKPin,0);  

  b = 0;
  for (i=0; i<8; i++) {
    b = b>>1;
    if (digitalRead(IOPin)) {
      b = b + 0x80;
    } 
   digitalWrite(CLKPin,1);    
 //  delayMicroseconds(Clk_delay);    
   digitalWrite(CLKPin,0);   
 //  delayMicroseconds(Clk_delay); 
  }

  digitalWrite(RSTPin,0);
  return(b);
}



byte    put_ds1302(byte b, byte c) // write ds1302 byte c at address b
{
byte i;

  digitalWrite(CLKPin,0);
  digitalWrite(RSTPin,1);
  pinMode(IOPin,OUTPUT);
  
  b = b<< 1;
  b &= ~1;
  b = b | 0x80;

  for (i=0; i<8; i++) {
    if ((b&0x01)!=0) {
     digitalWrite(IOPin,1);    
    } else {
     digitalWrite(IOPin,0);    
    }
    b = b>>1;
   digitalWrite(CLKPin,1);    
 //  delayMicroseconds(Clk_delay);    
   digitalWrite(CLKPin,0);   
 //  delayMicroseconds(Clk_delay); 
  }

  for (i=0; i<8; i++) {
    if ((c&0x01)!=0) {
     digitalWrite(IOPin,1);    
    } else {
     digitalWrite(IOPin,0);    
    }
    c = c>>1;
   digitalWrite(CLKPin,1);    
 //  delayMicroseconds(Clk_delay);    
   digitalWrite(CLKPin,0);   
 //  delayMicroseconds(Clk_delay); 
  }

  
  digitalWrite(RSTPin,0);   
}


//
//    BTOBCD - Binary to BCD
byte    btobcd(byte b)
{
byte c;
    c = (b/10)<<4;
    b=b%10;
    c += b;
    return(c);
}

void show(byte led, byte val) // used to display clock numbers
{
  ledmem[led--] = num7[val>>4];
  ledmem[led] = num7[val& 0x0f];  
}



const byte testclk[] = {00,00,12,20,9,2,11}; // set time in decimal

void  setclk(void) // set the clock to a known date
{
int i;

    for (i=0; i<7; i++) {
        rtc[i] = testclk[i];
    }

    rtc[DS_SEC] &= 0x7f; // by default, turn off clock halt flag in seconds
//    find_dow(); // set the day of the week
    
    if (rtc[DS_SEC]>=60) {
       rtc[DS_SEC] = 0x80;
    }
    
    put_ds1302(7,0);  // write enable
    for (i=0; i<7;i++) {
        put_ds1302(i,btobcd(rtc[i]));
    }
    put_ds1302(7,0xff);
}

void  read_clock(void) // read the clock from the ds1302 RTC
{
    rtc[DS_YEAR] = get_ds1302(DS_YEAR);
    rtc[DS_MON] = get_ds1302(DS_MON);
    rtc[DS_DAY] = get_ds1302(DS_DAY);
    rtc[DS_HOUR] = get_ds1302(DS_HOUR);
    rtc[DS_MIN] = get_ds1302(DS_MIN);
    rtc[DS_SEC] = get_ds1302(DS_SEC);
}

