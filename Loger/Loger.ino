//the start up fucntion also contains things it should not, need to move it back up

/*BioMapping Data Logger: Mobile Experience Lab,
 Symon Oliver. Bohdan Anderson. Myles Borins. Andrei Vasili
 
 This program communicates over I2C to an unasigned number of devices.
 The logic in this program is that we turn on all the communication int the setup function
 pay paticular attention to the openLogger setup function Startup and I2c testDevices function 
 if the logger is waiting to start recording it blinks really quickly
 once the button has been pushed, the logger will start recodring, the led blinks at a normal pace
 once the logger has been going for a while press the button again and it will stop logging
 
 */

//This is set up for the the Arduino pro mini with a serail microSD card and a 66 Channel LS20031 GPS 5Hz Receiver attached to the rx

//the middle connection wire goes to the top of the board

/*INIT GLOBAL VARIABLES*/

#include <Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>


/*******
 * GPS stuff
 * Arduino pin 3 is RX, goes to GPS TX
 * Arduino pin 4 is TX, goes to GPS RX
 *******/
TinyGPS gps;
//seting up the softserial for GPS
SoftwareSerial GPSSerial(3, 4); 
static void gpsdump(TinyGPS &gps);
//latitude, longiture, altitued
//get placed into this listt


float gpsList[3] = {
  1000,1000,1000};
boolean gpsNew = true; 
//when the GPS starts to get data the project starts recodring
boolean start = false;


/*******
 * logger setup
 * Arduino pin 5 is RX, goes to Openlogger TX
 * Arduino pin 6 is TX, goes to Openlogger RX
 *******/
SoftwareSerial logger(5,6);
String cbOpen = "{";
String cbClose = "}";

/*******
 * this is button ends the recording to the OpenLogger
 * once pressed it stops all recodring
 * Pin 10 starts off ON
 * as it logs it flashes
 * once endstate == true it will fade and turn on slowly
 *******/
boolean endbutton = false;
byte endubuttonVal = 7;
byte outPutLed = 11;
//this is for the state of the end button
int buttonstate = 255;


/*******
 * Devices
 * this list off the devices 
 * right now it's set for 5 (0-4)
 *******/
byte maxDevice = 4;
//manually have to change the values
byte list[5] = {   //add 1 to the maxDevise and put them in there
  0,0,0,0,0};      //add the zeros to the appropiate ammount
int inputValues[5] = {
  0,0,0,0,0};  


/*******
 * Genral set ups
 *******/
//this is save a little bit of memory
//instead of stating a each for loop we do it once
int a;
int Gcounter = 0; // is the global counter
int totalZero = 0;
byte serialIn = 0;





///////////////////////////////////////////////////
///////////////// setup ///////////////////////////
///////////////////////////////////////////////////
void setup() 
{
  //status LED turns on
  pinMode(outPutLed, OUTPUT);  
  digitalWrite(outPutLed, HIGH);   

  //end state button
  pinMode(endubuttonVal, INPUT); 


  //for 2IC communications
  Wire.begin();

  Serial.begin(9600);



  //this is for communication to the GPS
  GPSsetup();
  GPSSerial.begin(4800); 
  GPSSerial.listen();

  //  Startup();   //with no testing during start up
  //fills the list of input values and list, by doing this we find all the devices  
  testAllVarialbes();
  //  logger.begin(9600); //might not need this
}





///////////////////////////////////////////////////
///////////////// loop ////////////////////////////
///////////////////////////////////////////////////
void loop(){
  comms();
  if(endbutton == false){ //endbutton has not been pushed yet
    /*******
     * GPS data in
     * test to start recording on good data from gps
     *******/
    feedgps();    //this is loading the data into the GPS
    gpsdump(gps); //loading the GPS data and putting it into gpsList 0 being Long, 1 being Lat, 2 being altituted (but it's terible) 
      
    /*******
     * start recording when the button is pressed
     *******/
    if(start == false){
      buttonstate = digitalRead(endubuttonVal);  //set the value of the button
      if (buttonstate == 1){ //when it's grounded, make the button be off
          StartupTest();    
          testDevices();
          start = true;     
          logger.print(cbOpen);            
          delay(1000);
      }      
    }

    /*******
     * Main logging loop
     * here we get the data from all the devices
     * and then write them out
     * there is no value being passed into the readDevices becuase
     * you can't pass list in ardunio (it's c)
     * the list we are using are
     * "list"         to know what devices are available
     * this also carries what devices it is based on the 2nd digit
     * "inputValues"  to carry the values of the sensors associated with "list"
     * Gcounter number of times we have writen to the SD card
     * Right now we are waiting a total of 500milisec between each loggin
     * to change this change the delay times in here
     *******/
    if(start == true){ 
      //every 10 Gcounter we are going to recheck the devices   
      //change the value of 10 to change how offten it checks   
      if(Gcounter%10 == 1){
        testDevices();
      }
    

      readDevices();  //goes throug the devices on the "list" and places their vaules in "inputValues"  
      writeToLogger(Gcounter);// prints out to the logger the appropiate information

      //flash the led so we know it's recording
      delay(50);
      digitalWrite(outPutLed, HIGH);    
      delay(50);
      digitalWrite(outPutLed, LOW);    
      delay(50);
      digitalWrite(outPutLed, HIGH);    
      delay(50);
      digitalWrite(outPutLed, LOW); 
      delay(50);      
      //this records the number of times we have recorded
      Gcounter++;

      buttonstate = digitalRead(endubuttonVal);  //set the value of the button
      if (buttonstate == 1){ //when it's grounded, make the button be off
          endbutton = true;
          logger.print(cbClose);
      }
    }
    else {
      digitalWrite(outPutLed, LOW);    
      delay(10);
      digitalWrite(outPutLed, HIGH);    
      delay(10); 
    }     
  } 
  /*******
   * this is the end state,
   * the logger has stoped recording and now is waiting to be shut down
   * the led is beating.
   *******/
  else {
    while(true){
      comms();      
      // fade in from min to max in increments of 5 points:
      for(int fadeValue = 0 ; fadeValue <= 255; fadeValue +=3) { 
        // sets the value (range from 0 to 255):
        analogWrite(outPutLed, fadeValue);         
        // wait for 30 milliseconds to see the dimming effect    
        delay(30);                            
      } 

      // fade out from max to min in increments of 5 points:
      for(int fadeValue = 255 ; fadeValue >= 0; fadeValue -=2) { 
        // sets the value (range from 0 to 255):
        analogWrite(outPutLed, fadeValue);         
        // wait for 30 milliseconds to see the dimming effect    
        delay(30);                            
      } 
    }
  }   
}



//////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////// functions //////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
/*******
 * Prints the main part of the Json
 * ,"entryValue":{"time":timeValue,"heart":inputValue,"lat":gpsList[0],"log":gpsList[1],"alt":gpsList[2]}
 * the sensors are printed in case situation of them being presnet, and there are new valuse
 * if the gps in not new data then it will not be recorded  
 * we also test to make sure the GPS data is good data, in that it's not unrealistic
 *******/
void writeToLogger(int entryValue){
  logger.println(""); 
  if(entryValue != 0){ //when it's not the 1st one
    logger.print(", ");
  }
      
  
  logger.print('"');  
  logger.print(entryValue);   
  logger.print('"');

  logger.print(':'); 
  logger.print(cbOpen);
  logger.print('"');
  logger.print("time");
  logger.print('"');
  logger.print(':');  
  int time = millis();
  logger.print(time); 
  
  Serial.print("{");        
  Serial.print("time:");        
  Serial.print(time);          
  Serial.print(",");      



  //each device right now has a assigned address when programed
  //the second digit is what is used to determin the type of device, 4 or 14 or 24... = heart
  for(a = 0; a < maxDevice; ++ a){
    //going though the list 

    if(list[a] != 0 && inputValues[a] > 0){    
      logger.print(", ");    
      
      logger.print('"');
      //make the sensor number be between 0 and 10   
      byte sensor = shrink(list[a]);   
      //check for each case
      if(a != 0){
        Serial.print(",");                
      }
      if(sensor == 4){
        Serial.print("heart:");        
        logger.print("heart");   
      } 
      else if(sensor == 5){
        Serial.print("breath:");        
        logger.print("breath");           
      } 
      else if(sensor == 6){
        Serial.print("gsr:");        
        logger.print("gsr");
      }
      else if(sensor == 7){
        Serial.print("temp:");                
        logger.print("temp");
      } else {
        Serial.print("unkown:");                        
        logger.print("unkown");        
      }
      
      Serial.print(inputValues[a]);

      logger.print('"');
      logger.print(':');       
      logger.print(inputValues[a]);
 /*     if(a+1 < maxDevice){
        if(list[a+1] > 0){
          logger.print(',');
        }
      }      */
    }
    
  }

  //when there is a gps attached and there is actual data it will do this
  if(gpsList[0] < 181 && gpsNew == true){
    logger.print(','); 
    
    Serial.print(",gps{lat:");        
    Serial.print(gpsList[0]);        
    Serial.print(",lon:");        
    Serial.print(gpsList[1]);   
    Serial.print(",alt:");        
    Serial.print(gpsList[2]);   
    Serial.print("}");
    
    logger.print('"');
    logger.print("lat");  
    logger.print('"');
    logger.print(':');  
    logger.print(gpsList[0],int(7));  
    logger.print(',');  

    logger.print('"');
    logger.print("lon");  
    logger.print('"');
    logger.print(':');  
    logger.print(gpsList[1],int(7));  
    logger.print(','); 

    logger.print('"');
    logger.print("alt");  
    logger.print('"');
    logger.print(':');  
    logger.print(gpsList[2]); 

    gpsNew = false;    
  } 
  
  Serial.println("}");        
  
  logger.print(cbClose);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*******
 * Populate the gplList list.
 * this needs the GPS object as a variable in
 * uses a library to parse the data into a simple list
 * Again can't pass a list into this
 * but the list that it does affect is 
 * "gpsList"
 * O - being the lattitude
 * 1 - being the longatude
 * 2 - being the "alituted" - very inaccurate.
 * This function also whichs a booleaning gpsNew
 * letting the loger know that there is new data
 *******/
static void gpsdump(TinyGPS &gps)
{
  float flat, flon;
  unsigned long age;
  gps.f_get_position(&flat, &flon, &age);
  gpsList[0] = flat;
  gpsList[1] = flon;
  gpsList[2] = gps.f_altitude();
  
//  Serial.print("gps:[ ");
//  Serial.print(gpsList[0],9);
//  Serial.print(",");  
//  Serial.print(gpsList[1],9);
//  Serial.print(",");    
//  Serial.print(gpsList[2],9);  
//  Serial.println("]");    
  
  gpsNew = true;
}
/*******
 * Loads a GPS data into GPS object.
 * reads and loads the new GPS data into the the GPS object
 * this is also the only time we need to listen to the GPSSerial
 *******/
void feedgps()
{
  while (GPSSerial.available())
  {
    gps.encode(GPSSerial.read());
  }
}

/*******
to change the default settings of the gps to what we need
 *******/
void GPSsetup(){
  GPSSerial.begin(57600);
  delay(1000);
  GPSSerial.println("$PMTK300,1000,0,0,0,0*1C");   // 1Hz update rate
  delay(1000);
  GPSSerial.println("$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"); // report GGA every update
  delay(1000);
  GPSSerial.println("$PMTK251,4800*14");  // Use 4800 bps
  delay(1000);
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*******
 * To find the Devices on the I2C
 * populates the "list" with the adress of devices
 * if in the list is a 0 there is nothing there
 * the list right now is limited to 5 adress
 * to increase it at the top of this program change Devices
 *******/
void testDevices(){
  byte testDevicesCounter = 0;  
  for(a = 2; a < 100; ++a){  
    Wire.requestFrom(a, 2);

    if (Wire.available())
    { 
      int c = Wire.read();
      list[testDevicesCounter] = a;
      testDevicesCounter++;
    } 
  }
}
/*******
 * Used to Populate the inputValues from the list
 * request from each device with their most recent value
 *******/
void readDevices(){  
  //this fuction saves the inputs from i2c into the imputValues list.
  for(a = 0; a < maxDevice; ++ a){
    if(list[a] != 0){
      inputValues[a] = writeI2c(list[a]);          
    }
  }
}
/*******
 * writes to I2c device which is passed in as board
 * requests 2 bytes from the device
 * and returns (value1 * 250) + value2
 * on the i2c devices we devide the value input by 250 and that is the frist byte
 * the 2nd byte is the remainder   
 *******/
int writeI2c(int board){
  Wire.requestFrom(board, 2);
  boolean val1Written = false;
  int val1 = 0;
  int val2 = 0;
  while (Wire.available())
  { 
    if(val1Written == false){
      val1 = Wire.read(); // receive a byte as character    
      val1Written = true;
    } 
    else {
      val2 = Wire.read();
    }
  }  
  return (val1*250)+val2;
}


/*******
 * To remove the 1st digit and return the 2nd
 * used to find out what device we are talking to
 * the 2nd digit holds the name of the i2c device
 *******/
byte shrink(byte localIn){
  while (true) {
    if ( localIn > 10) {
      localIn = localIn -10;
    } 
    else {
      break;
    }
  }
  return(localIn);
}




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/*******
 * Starts up the OpenLoger via the example programs
 * at the moment it doesn't need to write to it to enter command mode 
 * because it should automatically start in command mode
 * 
 *******/
char buff[50];

void StartupTest(){   
  testAllVarialbes();

  Serial.println("Open loger test sequence of 2");

  //waiting for the logger to start
  logger.begin(9600);

  //we are now listening to the logger
  //not the GPS
  logger.listen();
  delay(1000);  

  logger.write(13);


  //Works with Arduino v1.0
  logger.write(36);
  logger.write(36);
  logger.write(36);

  //Wait for OpenLog to respond with '>' to indicate we are in command mode
  while(1) {
    if(logger.available())
      if(logger.read() == '>') break;
  }

  //Old way
  sprintf(buff, "new Data.txt\r");
  logger.print(buff); //\r in string + regular print works with older v2.5 Openlogs

  //Wait for OpenLog to return to waiting for a command
  while(1) {
    if(logger.available())
      if(logger.read() == '>') break;
  }
  Serial.println("1 of 2 - make file");

  sprintf(buff, "append Data.txt\r");
  logger.print(buff);

  //Wait for OpenLog to indicate file is open and ready for writing
  while(1) {
    if(logger.available())
      if(logger.read() == '<') break;
  }

  Serial.println("2 of 2 - append file");  
  Serial.println("ready to be written to");

  //Begging the recording to the SD card
  GPSSerial.listen();
  
}


/*******
 * This test all the global variables, 
 * Allowing for easier trouble shooting
 *******/
void testAllVarialbes(){
  Serial.print("gpsList "); 
  Serial.print(gpsList[0],9);
  Serial.print(" ");
  Serial.print(gpsList[1],9);
  Serial.print(" ");
  Serial.println(gpsList[2],9);

  Serial.print("gpsNew ");  
  Serial.print(gpsNew);
  Serial.print(" new data."); 

  Serial.print("Start ");
  Serial.print(start);  
  Serial.println(" from gps data.");  

  Serial.print("endbutton ");
  Serial.print(endbutton);
  Serial.print(" state from pin ");
  Serial.println(endubuttonVal);

  Serial.print("outPutLed on pin ");
  Serial.println(outPutLed);

  Serial.print("buttonstate ");
  Serial.println(buttonstate);  

  testDevices();

  Serial.print("maxDevices on i2c ");
  Serial.println(maxDevice);

  Serial.print("list of devices ");
  Serial.print(list[0]);
  Serial.print(" ");  
  Serial.print(list[1]);
  Serial.print(" ");  
  Serial.print(list[2]);
  Serial.print(" ");  
  Serial.print(list[3]);
  Serial.println(" "); 

  readDevices(); 

  Serial.print("imputValues ");
  Serial.print(inputValues[0]);
  Serial.print(" ");  
  Serial.print(inputValues[1]);
  Serial.print(" ");
  Serial.print(inputValues[2]);
  Serial.print(" ");
  Serial.print(inputValues[3]);
  Serial.println(" ");  

  Serial.print("Gcounter ");
  Serial.println(Gcounter);

  Serial.print("totalZero ");
  Serial.println(totalZero);

  Serial.print("a ");
  Serial.println(a);

  Serial.print("serialIn ");
  Serial.println(serialIn);

}


/*******
 * Test for the serial input
 * if t is pressed...
 * if o is pressed
 * we communicate with the open logger
 * 
 *******/
void comms(){
  if(Serial.available() > 0){
    serialIn = Serial.read();    
    if(serialIn == 't'){
      Serial.println("Testing All variables");
      testAllVarialbes();
    }
    if(serialIn == 'o'){
      while(true){
        if(Serial.available()){
          logger.write(Serial.read());
        }  
        if(logger.available()){
          Serial.write(logger.read());    
        }       
      }      
    }
  }  
}
















































