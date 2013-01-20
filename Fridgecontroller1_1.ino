

#include <OneWire.h>
#include <DallasTemperature.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

// Temperature sensor and compressor setup
#define ONEWIRE 7
#define DISABLE 2
#define COMPRESSOR 3

#define FRIDGEHIGHTEMP 3.0
#define FRIDGELOWTEMP 2.5

#define MAXRUNTIME 1200 // 840 //14 minutes maximum runtime
#define WARMUP 840 //14 minutes initial startup - avoids reverse pressure problems apparently
#define COOLDOWN 840 //14 mins cooldown
#define UPDATE_FREQ 15000;

//uint8_t fridge_address[8] = {0x10, 0xfa, 0x47, 0x35, 0x00, 0x00, 0x00, 0x37};

// 220L Kelvinator is 85 watts
#define COMPRESSOR_WATTAGE 85.0

// It is recommended to not turn the compressor on immediately after startup
// because back pressure can damage the compressor
//#define COMPRESSOR_STARTUP_DELAY 120
#define COMPRESSOR_STARTUP_DELAY 0

// How long between measurement cycles
#define SLEEP_SEC 10


//Now radio

// nRF24L01(+) radio attached using Getting Started board 
RF24 radio(9,10);

// Network uses that radio
RF24Network network(radio);

// Address of our node
uint16_t this_node = 3;

// Address of the other node
const uint16_t other_node = 1;



OneWire oneWire(ONEWIRE);
DallasTemperature sensors(&oneWire);

unsigned long runtime = 0, chilltime = 0, last_checked = 0, this_check = 0;
int start_compressor = 1;
uint8_t compressor = HIGH;



void setup()   {                
  // initialize the digital pin as an output:
  pinMode(COMPRESSOR, OUTPUT);
  pinMode(DISABLE, OUTPUT);
  digitalWrite(COMPRESSOR, HIGH);
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 90, /*node address*/ this_node);

  
  
  Serial.begin(115200);
  Serial.println("FridgeControlWeb");
  sensors.begin();
  sensors.requestTemperatures();
  float fridge = sensors.getTempCByIndex(0);
  
 Serial.println(fridge);
  
}

  int i, j, data_inset, delta;
  char float_conv[10];
  float t, fridge = 5.0, freezer = -20.0;
  int run_counter = 0;
  int cool_down = COOLDOWN;

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
 // Serial.println("Warmup complete");
  start_compressor = 0;
 }  
//  Serial.print("start compressor = ");
//  Serial.print(start_compressor);
//  Serial.print(" Fridge temp = ");
//  Serial.println(fridge);




 if(run_counter > MAXRUNTIME)
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
   Serial.println("Warmup complete or Hit inside runcounter zero statement");
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
      //sprintf(data + data_inset, "Start delay remaining: %d\n",
    //          start_compressor);
     // data_inset = strlen(data);
      //digitalWrite(DISABLE, HIGH);
    }
 

    //last_checked = this_check;
    long check = last_checked + UPDATE_FREQ;

    if(millis() > check)
   {
     send_update(fridge, compressor);
     last_checked = millis();
   } 
    
    
    delay (1000);
  }
  
 void send_update(float temp, int state)
 {
 
  
 
  wd_payload_t wd_payload;


 wd_payload.beer_fridge = temp;
 wd_payload.compressor_state = state;
 
    RF24NetworkHeader wd_header(/*to node*/ 0);
    bool ok = network.write(wd_header,&wd_payload,sizeof(wd_payload));
    if (ok)
      Serial.println("ok.");
    else
      Serial.println("failed.");
   
 }    
   
 
 

