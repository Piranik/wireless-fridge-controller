/* 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    ------
    
    Note relay attached to pin 3 has HIGH as off and LOW as on
    
    
*/

#include <OneWire.h>            // Required for DS18B20
#include <DallasTemperature.h>  // Required for DS18B20

  
#include <RF24.h>               // Required for RF24 Network
#include <SPI.h>                // Required for RF24
#include <RF24Network.h>        // Brilliant networking library for nordic 2.4ghz cheap as chips dongles


// Temperature sensor and compressor setup
#define ONEWIRE 7

#define DISABLE 2          // Not used, relic of previous code
#define COMPRESSOR 3       // Compressor pin connected to mechanical relay

#define FRIDGEHIGHTEMP 3.0   // Statically assigned now, future updates will have variables set remotely
#define FRIDGELOWTEMP 2.5    // Statically assigned now, future updates will have variables set remotely

#define MAXRUNTIME 1200 // Define maximum runtime as 20 minutes, much longer than this and the fridge performance drops significantly
#define WARMUP 840 //14 minutes initial startup - avoids reverse pressure problems called out on hackaday
#define COOLDOWN 840 //14 mins cooldown - no reason for 14 minutes, may reduce 

#define UPDATE_FREQ 15000;

// How long between measurement cycles
#define SLEEP_SEC 10

//Now define radio

// nRF24L01(+) radio attached with standard pins (eg getting started board)
RF24 radio(9,10);

// Network uses that radio
RF24Network network(radio);

// Address of our node
uint16_t this_node = 3; // node 3 is the fridge, node 0 is the root node

// Address of the other node
const uint16_t root_node = 0;



OneWire oneWire(ONEWIRE);
DallasTemperature sensors(&oneWire);

unsigned long runtime = 0, chilltime = 0, last_checked = 0, this_check = 0;
int start_compressor = 1;
uint8_t compressor = HIGH;


void setup()   {                
  // initialize the digital pins as an output:
  pinMode(COMPRESSOR, OUTPUT);
  pinMode(DISABLE, OUTPUT);
  
  // Ensure relay starts in OFF position so we can wait for 'warmup'
  
  digitalWrite(COMPRESSOR, HIGH);
  
  // Start up the RF24network
  
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);

  
  
  Serial.begin(115200);
  Serial.println("Wireless-Fridge-Controller");
  
  // Initiate DS18B20
  
  sensors.begin();
  
  
 /* Useful for debugging - checks that we're getting temperature readings straight away
 
  sensors.requestTemperatures();
  float fridge = sensors.getTempCByIndex(0);
  Serial.println(fridge);
  
  */
}

  float fridge = 5.0; 
  
  int run_counter = 0;

// Define structure for sending data to root node - root node receives and passes to PC for uploads to Cosm. 

    struct wd_payload_t
{
  float beer_fridge;
  int compressor_state;
};


void loop()                     
{

  network.update();
  sensors.requestTemperatures();
  fridge = sensors.getTempCByIndex(0);


 // WAIT FOR WARMUP PERIOD
 
 if(millis()/1000 > WARMUP && start_compressor == 1)
 {
  start_compressor = 0;
 }  

// Will tidy up these sections later, I am sure there is a much more elegant way of doing this



 if(run_counter > MAXRUNTIME)  // If max run time hit, initiate a cool-down sequence.
 {
   start_compressor = 2;
   Serial.println("Run for 14 mins, time to cool down");
   
   compressor = HIGH;
   digitalWrite(COMPRESSOR, compressor);
   run_counter = COOLDOWN;

 }
 
 if(start_compressor == 2)
 {
   Serial.print("Cooling down: ");
   Serial.println(run_counter);
   run_counter= run_counter - 1;
 }
 
 if(run_counter == 0 && millis()/1000 > WARMUP)
 {
   Serial.println("Warmup complete and/or Hit inside runcounter zero statement");
   start_compressor = 0;
 }



    // Control compressor
    if(start_compressor < 1)
    {
      digitalWrite(DISABLE, HIGH);
 
      if(fridge > FRIDGEHIGHTEMP )
      {
        
        compressor = LOW;
        Serial.println("Turning compressor on");
        Serial.print("Temp: ");
        Serial.println(fridge);
        Serial.print("run_counter = ");
        Serial.println(run_counter);
        run_counter++;
        digitalWrite(COMPRESSOR, compressor);

      }
      else if(fridge < FRIDGELOWTEMP)
      {
        compressor = HIGH;
        Serial.println("Turning compressor off");
        Serial.print("Temp: ");
        Serial.println(fridge);
        digitalWrite(COMPRESSOR, compressor);

        run_counter = 0;
      }
      digitalWrite(COMPRESSOR, compressor);
    }
    else
    {

    }
 

    //last_checked = this_check;
    long check = last_checked + UPDATE_FREQ;

    if(millis() > check)
   {
     send_update(fridge, compressor);
     last_checked = millis();
   } 
    
    
    delay (1000);  // On the to-do list is to remove this and move to a more exact 'millis' based system. 
  }
  
  
 
 // function to send update to root node
  
 void send_update(float temp, int state)
 {
 
  wd_payload_t wd_payload;


 wd_payload.beer_fridge = temp;
 wd_payload.compressor_state = state;
 
    RF24NetworkHeader wd_header(/*to node*/ root_node);
    bool ok = network.write(wd_header,&wd_payload,sizeof(wd_payload));
    if (ok)
      Serial.println("ok.");
    else
      Serial.println("failed.");
   
 }    
   
 
 

