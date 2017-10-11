
/***************************************************************************************************
 Project: DecoTime angular tea timer
 By: Andy T. Evans 
 Date: 8/12/2015 - 10/10/2017
 Website: http:\\www.coolate.com
 Description: This is a retro tea timer that usese speedometer steppers (switecx25) to show the time. 
 It uses two rotery encoders with push buttons to set the time, and a number
 of leds to show the percent left. When finished it will ring a bell and 
 flash an LED. It does this all via a MCP23S17 chip. It also has preset buttons that can 
 be held to save the current values to that button.
 
 INCLUDES: 
 -MCP23S17 - http://playground.arduino.cc/Main/MCP23S17
 -UP/Down Timer - http://playground.arduino.cc/Main/CountUpDownTimer
 -switecX25 - https://github.com/clearwater/SwitecX25
 -OneButton - https://github.com/mathertel/OneButton   
 -SPI - https://www.arduino.cc/en/Reference/SPI
 -EEPROM - https://www.arduino.cc/en/Reference/EEPROM
****************************************************************************************************/

#include "SwitecX25.h"
#include "CountUpDownTimer.h"
#include <SPI.h>              // We use this library, so it must be called here.
#include <MCP23S17.h>         // Here is the new class to make using the MCP23S17 easy.
#include <OneButton.h>       // handy button lib, does debounce and long press making code easy to read
#include <EEPROM.h>          // Lets us save changed presets

//Rotary Encoder Max/Min
#define maximum 255
#define minimum 0

#define encoderMinPinA 3  //ENCODER1
#define encoderMinPinB 4  //ENCODER1
#define encoderSecPinA 2  //ENCODER2
#define encoderSecPinB 1  //ENCODER2
#define startStopPin 5
#define resetTimerPin 6
#define ledDone 7
#define bellPin 8
#define led1 9
#define led2 10
#define led3 11
#define led4 12
#define led5 13
#define led6 14
#define led7 15
#define led8 16

long ledPreviousMillis = 0;        // will store last time LED was updated
long ledInterval = 400;
int ledDoneState = HIGH;

//preset buttons, using analog pins, and a digital. Ideally one could set up all 5 buttons on one alalog by using resistors. 
OneButton presetButton1(A1, true);    //analog 1
OneButton presetButton2(A2, true);
OneButton presetButton3(A3, true);
OneButton presetButton4(1, true);     //digital 0, needed the analog 4 for SDA on expantion port
OneButton presetButton5(0, true);     //digital 1, needed the analog 5 for SCL on expantion port

//way to make dividing less processor intensive
int led12Percent = 0; 
int led25Percent = 0;
int led37Percent = 0;
int led50Percent = 0;
int led62Percent = 0;
int led75Percent = 0;
int led87Percent = 0;
int led95Percent = 0;

int startStopButtonState = 0;         // current state of the button
int startStopLastButtonState = 0;     // previous state of the button
int resetTimerButtonState = 0;         
int resetTimerLastButtonState = 0; 

int bellRings = 3;
int bellRingCount = 0;

int encoderMinPinALast = LOW;
int n = LOW;
int encoderMinPos = 0;
int encoderMinPosLast = 0;

int encoderSecPinALast = LOW;
int o = LOW;
float encoderSecPos = 0;
int encoderSecPosLast = 0;

// 315 degrees of range = 315x3 steps = 945 steps
// declare motor1 with 945 steps on pins 4-7
SwitecX25 motor1(315*3, 5,4,2,3); 
SwitecX25 motor2(315*3, 6,7,9,8); 

MCP iochip(0);             // Instantiate an object called "iochip" on an MCP23S17 device at address 0

//make a timer
CountUpDownTimer theClock(DOWN);

int timerGoing = 0;
int timerSet = 0;
float min = 9;
float sec = 30;
int totalSec = 0;
int currentTotalSec = 0;

//set the default presets here, they can be over written when 
//button held down and new value saved.
byte preSetMin1 = 0;
byte preSetSec1 = 30;
byte preSetMin2 = 2;
byte preSetSec2 = 15;
byte preSetMin3 = 3;
byte preSetSec3 = 30;
byte preSetMin4 = 5;
byte preSetSec4 = 10;
byte preSetMin5 = 15;
byte preSetSec5 = 20;

//version number really only used for EEPROM checking, make it different to re-write default presets.
//1-254 do not pick 0 or 255 as they can mean "blank" in some peoples use of the EEPROM
byte decoVersion = 49;
byte decoVersionSaved = 0;
int versionAddress = 211;

//================================ SETUP ===================================================================  
void setup() {

  //load decoVersion from EEPROM 
  decoVersionSaved = EEPROM.read(versionAddress);
  
  iochip.pinMode(1, HIGH);      // Use bit-write mode to set the pin as an input (inputs are logic level 1)
  iochip.pullupMode(1, HIGH);   // Use bit-write mode to Turn on the internal pull-up resistor on the pin
  iochip.inputInvert(1, HIGH);  // Use bit-write mode to invert the input so that logic 0 is read as HIGH
    
  iochip.pinMode(2, HIGH);      // Use bit-write mode to set the pin as an input (inputs are logic level 1)
  iochip.pullupMode(2, HIGH);   // Use bit-write mode to Turn on the internal pull-up resistor on the pin
  iochip.inputInvert(2, HIGH);  // Use bit-write mode to invert the input so that logic 0 is read as HIGH
  
  iochip.pinMode(3, HIGH);      // Use bit-write mode to set the pin as an input (inputs are logic level 1)
  iochip.pullupMode(3, HIGH);   // Use bit-write mode to Turn on the internal pull-up resistor on the pin
  iochip.inputInvert(3, HIGH);  // Use bit-write mode to invert the input so that logic 0 is read as HIGH
    
  iochip.pinMode(4, HIGH);      // Use bit-write mode to set the pin as an input (inputs are logic level 1)
  iochip.pullupMode(4, HIGH);   // Use bit-write mode to Turn on the internal pull-up resistor on the pin
  iochip.inputInvert(4, HIGH);  // Use bit-write mode to invert the input so that logic 0 is read as HIGH
  
  iochip.pinMode(startStopPin, HIGH);      // Use bit-write mode to set the pin as an input (inputs are logic level 1)
  iochip.pullupMode(startStopPin, HIGH);   // Use bit-write mode to Turn on the internal pull-up resistor on the pin
  iochip.inputInvert(startStopPin, HIGH);  // Use bit-write mode to invert the input so that logic 0 is read as HIGH
  
  iochip.pinMode(resetTimerPin, HIGH);      // Use bit-write mode to set the pin as an input (inputs are logic level 1)
  iochip.pullupMode(resetTimerPin, HIGH);   // Use bit-write mode to Turn on the internal pull-up resistor on the pin
  iochip.inputInvert(resetTimerPin, HIGH);  // Use bit-write mode to invert the input so that logic 0 is read as HIGH
  
  iochip.pinMode(led1, LOW);       // Use bit-write mode to set the current pin to be an output
  iochip.pinMode(led2, LOW);       
  iochip.pinMode(led3, LOW);
  iochip.pinMode(led4, LOW);
  iochip.pinMode(led5, LOW);
  iochip.pinMode(led6, LOW);
  iochip.pinMode(led7, LOW);
  iochip.pinMode(led8, LOW);
  iochip.pinMode(ledDone, LOW);
  iochip.pinMode(bellPin, LOW); 
  
  //old way not using oneButton.h
  //pinMode(presetButton1pin, INPUT_PULLUP);
  
   // link the presetButton 1 functions.
  presetButton1.attachClick(pressPresetButton1);
  //presetButton1.attachDoubleClick(doubleclick1);
  presetButton1.attachLongPressStart(longPressStart);
  presetButton1.attachLongPressStop(longPressStopPresetButton1);
  //presetButton1.attachDuringLongPress(longPress1);
  presetButton1.setPressTicks(3000);   //sets the long click to 3 sec

  presetButton2.attachClick(pressPresetButton2);
  presetButton2.attachLongPressStart(longPressStart);
  presetButton2.attachLongPressStop(longPressStopPresetButton2);
  presetButton2.setPressTicks(3000);   //sets the long click to 3 sec

  presetButton3.attachClick(pressPresetButton3);
  presetButton3.attachLongPressStart(longPressStart);
  presetButton3.attachLongPressStop(longPressStopPresetButton3);
  presetButton3.setPressTicks(3000);   //sets the long click to 3 sec

  presetButton4.attachClick(pressPresetButton4);
  presetButton4.attachLongPressStart(longPressStart);
  presetButton4.attachLongPressStop(longPressStopPresetButton4);
  presetButton4.setPressTicks(3000);   //sets the long click to 3 sec

  presetButton5.attachClick(pressPresetButton5);
  presetButton5.attachLongPressStart(longPressStart);
  presetButton5.attachLongPressStop(longPressStopPresetButton5);
  presetButton5.setPressTicks(3000);   //sets the long click to 3 sec

  //will make sure epprom values for the preset buttons have been set with the defaults, if not manually set.
  updatePresetEEPROM();
  
  //Serial.begin (115200);
  
  //displays a test and syncs the servos
  startUpTest();
  
}

//++++++++++++++++++++++++++++++++++++ LOOP +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
void loop() {
  // update motors frequently to allow them to step
  motor1.update();
  motor2.update();
  presetButton1.tick();
  presetButton2.tick();
  presetButton3.tick();
  presetButton4.tick();
  presetButton5.tick();
 
//=========================== timer logic ===================================================================   
  if(timerGoing==1)
  {
    theClock.Timer(); // run the timer
    if (theClock.TimeHasChanged() ) // this prevents the time from being constantly shown.
        {
          motor1.setPosition(abs(15.75*theClock.ShowSeconds()));
          // motor2.setPosition(900);
          motor2.setPosition(calculateMinPos(theClock.ShowMinutes()));
          //motor1.updateBlocking();
          //motor2.updateBlocking();
          
          currentTotalSec = ((int)theClock.ShowMinutes()*60) + (int)theClock.ShowSeconds();
          //Serial.print(theClock.ShowHours());
          //Serial.print(":");
          //Serial.print(theClock.ShowMinutes());
          //Serial.print(":");
          //Serial.print(theClock.ShowSeconds());
          //Serial.print(":");
          //Serial.println(theClock.ShowMilliSeconds());
          // This DOES NOT format the time to 0:0x when seconds is less than 10.
          // if you need to format the time to standard format, use the sprintf() function.
        }
  }
  else
  {
    sec = encoderSecPos;
    min = calculateMinEncoderVal(encoderMinPos);   //255 max set earlyer
    motor1.setPosition(abs(15.75*sec));
    motor2.setPosition(calculateMinPos(min));
   // motor1.updateBlocking();
    //motor2.updateBlocking();
  }
  
//=========================== ENCODER reading ===================================================================    
  n = iochip.digitalRead(encoderMinPinA);

 
  if ((encoderMinPinALast == LOW) && (n == HIGH)) {
    if (iochip.digitalRead(encoderMinPinB) == LOW) {
      if(encoderMinPos <= minimum) {
        encoderMinPos = minimum;
      }
      else {
        encoderMinPos--;
      }
    }
    else {
      if(encoderMinPos >= maximum) {
        encoderMinPos = maximum;
      }
      else {
        encoderMinPos++;
      }
    }
  }
 
  if(encoderMinPos != encoderMinPosLast)
  {
    //Serial.println ((String)encoderMinPos);
    encoderMinPosLast = encoderMinPos;
  }
  encoderMinPinALast = n;
  
  o = iochip.digitalRead(encoderSecPinA);

 
  if ((encoderSecPinALast == LOW) && (o == HIGH)) {
    if (iochip.digitalRead(encoderSecPinB) == LOW) {
      if(encoderSecPos <= minimum) {
        encoderSecPos = minimum;
      }
      else {
        encoderSecPos--;
      }
    }
    else {
      if(encoderSecPos >= maximum) {
        encoderSecPos = maximum;
      }
      else {
        encoderSecPos++;  
      }
    }
  }
  
 
  if(encoderSecPos != encoderSecPosLast)
  {
    //Serial.println (encoderSecPos);
    encoderSecPosLast = encoderSecPos;
  }
  encoderSecPinALast = o;
  
//=========================== BUTTONS ===================================================================  
  startStopButtonState = iochip.digitalRead(startStopPin);

  // compare the startStopButtonState to its previous state
  if (startStopButtonState != startStopLastButtonState) {
    // if the state has changed, increment the counter
    if (startStopButtonState == HIGH) {
      // if the current state is HIGH then the button
      // went from off to on:
      if(timerSet == 0){
        timerSet = 1;
        timerGoing = 1;
        ledDoneState = LOW;
        iochip.digitalWrite(ledDone, ledDoneState);
        theClock.SetTimer(0,abs((int)min),abs((int)sec));
        totalSec = ((int)min*60) + (int)sec;
        calculatePercents(totalSec);
        theClock.StartTimer();
      }
      else{
         if (timerGoing == 0){
            theClock.ResumeTimer();
            timerGoing = 1;
         }
        else{
         theClock.PauseTimer();
         timerGoing = 0;
        } 
      }
      
    } 
    else {
      // if the current state is LOW then the button
      // went from on to off:
      
    }
  }
  startStopLastButtonState = startStopButtonState;
  
   resetTimerButtonState = iochip.digitalRead(resetTimerPin);

  // compare the startStopButtonState to its previous state
  if (resetTimerButtonState != resetTimerLastButtonState) {
    // if the state has changed, increment the counter
    if (resetTimerButtonState == HIGH) {
      // if the current state is HIGH then the button
      // went from off to on:
       timerGoing = 0;
       bellRingCount = 0;
      //h,m,s or sec
      theClock.SetTimer(0,abs((int)min),abs((int)sec));
      totalSec = ((int)min*60) + (int)sec;
      calculatePercents(totalSec);
      theClock.PauseTimer();
      
    } 
    else {
      // if the current state is LOW then the button
      // went from on to off:
       
    }
  }
 resetTimerLastButtonState = resetTimerButtonState;
 

 //=========================== The end of TIME =================================================================== 
 if(theClock.TimeCheck(0,0,0) && timerSet == 1){
   
   unsigned long currentMillis = millis();
 
  if(currentMillis - ledPreviousMillis > ledInterval) {
    // save the last time you blinked the LED 
    ledPreviousMillis = currentMillis;   

    // if the LED is off turn it on and vice-versa:
    if (ledDoneState == LOW)
      ledDoneState = HIGH;
    else
      ledDoneState = LOW;

    // set the LED with the ledState of the variable:
    iochip.digitalWrite(ledDone, ledDoneState);
    if(ledInterval > 400){
      ledInterval = ledInterval * .5;
    }
    else{
      ledInterval = ledInterval * 2;
    }
  }
  
   //ring my bell bell bell, ring my bell
    if(bellRingCount < bellRings)
    {
      iochip.digitalWrite(bellPin, HIGH);
      bellRingCount++;
      delay(5);
      iochip.digitalWrite(bellPin, LOW);
      motor1.updateBlocking();
      motor2.updateBlocking();
      delay(2000);
    }
   
 }
 
 //=========================== LEDs ===================================================================  
  if(currentTotalSec > led12Percent){  //led12Percent
    iochip.digitalWrite(led1, HIGH);
  }
  else{
    iochip.digitalWrite(led1, LOW);
  }
  if(currentTotalSec > led25Percent){
    iochip.digitalWrite(led2, HIGH);
  }
  else{
    iochip.digitalWrite(led2, LOW);
  }
  if(currentTotalSec > led37Percent){
    iochip.digitalWrite(led3, HIGH);
  }
  else{
    iochip.digitalWrite(led3, LOW);
  }
  if(currentTotalSec > led50Percent){
    iochip.digitalWrite(led4, HIGH);
  }
  else{
    iochip.digitalWrite(led4, LOW);
  }
  if(currentTotalSec > led62Percent){
    iochip.digitalWrite(led5, HIGH);
  }
  else{
    iochip.digitalWrite(led5, LOW);
  }
  if(currentTotalSec > led75Percent){
    iochip.digitalWrite(led6, HIGH);
  }
  else{
    iochip.digitalWrite(led6, LOW);
  }
  if(currentTotalSec > led87Percent){
    iochip.digitalWrite(led7, HIGH);
  }
  else{
    iochip.digitalWrite(led7, LOW);
  }
  if(currentTotalSec > led95Percent){
    iochip.digitalWrite(led8, HIGH);
  }
  else{
    iochip.digitalWrite(led8, LOW);
  }
  
  
}
 //=========================== percent helper  =================================================================== 
 //this will save processor cycles by not having to do float div to get the % done
void calculatePercents(int totalSeconds){
 led12Percent = (totalSeconds*12)/100; //10% 
 led25Percent = (totalSeconds*25)/100;
 led37Percent = (totalSeconds*37)/100;
 led50Percent = (totalSeconds*50)/100;
 led62Percent = (totalSeconds*62)/100;
 led75Percent = (totalSeconds*75)/100;
 led87Percent = (totalSeconds*87)/100;
 led95Percent = (totalSeconds*95)/100;
  
}

//=========================== calculateMinPos  =================================================================== 
 //this will take time in min and turn it into a value for servo in the range of 0-312
 //315 degrees of range = 315x3 steps = 945 steps
int calculateMinPos(int currentMin){
    int stepValue=0;
    if (currentMin<=5){            //for first 1/4 covered by 5 min
        stepValue= abs((int)currentMin*47);      
    }
    else if (currentMin > 5 && currentMin <=35){    //for middle 1/2
        stepValue = abs((int)236.25 + (15.75*(currentMin-5))); 
    }
    else if (currentMin>35){
        stepValue = abs((int)708.75 + (9.45*(currentMin-35)));
    }
    return stepValue;
 }

//=========================== calculateMinEncoderVal  =================================================================== 
 //this will take the encoder value and turn in into min for the dial I made. The first 5 min take more turn then the next
 //and so forth.
 //255
int calculateMinEncoderVal(int encoderPos){
    int minVal=0;
    
    if (encoderPos<=40){            //for first 1/4 covered by 5 min
        minVal= encoderPos/8;      
    }
    else if (encoderPos > 40 && encoderPos <= 160){    //for middle 1/2 5-35 min
        minVal = (encoderPos-20)/4;
    }
    else if (encoderPos>160){           //for 35 over
        minVal = (encoderPos-55)/3;
    }
    
    //minVal = encoderPos/4;
    return minVal;
 }

//=========================== setEncodersPos  =================================================================== 
 //This will set the encoders positions to the current values
 // this will let the preset buttons be able to be tweaked without going back to the last encoder locations
 // the min encoder math is more complicated, see above function for reverse.
 // 
void setEncodersPos(int newMin, int newSec){
    encoderSecPos = newSec;
    encoderSecPosLast= newSec;
    if (newMin<=5){            //for first 1/4 covered by 5 min
        encoderMinPos = (newMin *8);
        encoderMinPosLast = (newMin *8);     
    }
    else if (newMin > 5 && newMin <=35){    //for middle 1/2
        encoderMinPos = (newMin *4)+20;
        encoderMinPosLast = (newMin *4)+20;
    }
    else if (newMin>35){
        encoderMinPos = (newMin *3)+55;
        encoderMinPosLast = (newMin *3)+55;
    }
    
 }

 //=========================== startUpTest  =================================================================== 
 // This will run the start up test to see if all the LEDs work and center the servos
 // 
 // 
 // 
void startUpTest(){
  int ledDelay = 50;
    // run both motors against stops to re-zero
  motor1.zero();   // this is a slow, blocking operation
  motor2.zero();
  
  motor1.setPosition(945);
  motor2.setPosition(945);
  
  motor1.updateBlocking();
  motor2.updateBlocking();
  
  iochip.digitalWrite(led1, HIGH);
  delay(ledDelay);
  iochip.digitalWrite(led2, HIGH);
  delay(ledDelay);
  iochip.digitalWrite(led3, HIGH);
  delay(ledDelay);
  iochip.digitalWrite(led4, HIGH);
  delay(ledDelay);
  iochip.digitalWrite(led5, HIGH);
  delay(ledDelay);
  iochip.digitalWrite(led6, HIGH);
  delay(ledDelay);
  iochip.digitalWrite(led7, HIGH);
  delay(ledDelay);
  iochip.digitalWrite(led8, HIGH);
  delay(ledDelay);
  iochip.digitalWrite(ledDone, ledDoneState);
  delay(ledDelay);
  iochip.digitalWrite(led1, LOW);
  delay(ledDelay);
  iochip.digitalWrite(led2, LOW);
  delay(ledDelay);
  iochip.digitalWrite(led3, LOW);
  delay(ledDelay);
  iochip.digitalWrite(led4, LOW);
  delay(ledDelay);
  iochip.digitalWrite(led5, LOW);
  delay(ledDelay);
  iochip.digitalWrite(led6, LOW);
  delay(ledDelay);
  iochip.digitalWrite(led7, LOW);
  delay(ledDelay);
  iochip.digitalWrite(led8, LOW);
  
  motor1.setPosition(0);
  motor2.setPosition(0);
  
  motor1.update();
  motor2.update();
    
 }

 //=========================== PreSet Button functions =================================================================== 
 //This will set the encoders positions to the current values
 
void pressPresetButton1(){
    if(timerGoing == 0){
       min = preSetMin1;
       sec = preSetSec1;
       setEncodersPos(min, sec);  
     }
}
void pressPresetButton2(){
    if(timerGoing == 0){
       min = preSetMin2;
       sec = preSetSec2;
       setEncodersPos(min, sec);  
    }
}
void pressPresetButton3(){
    if(timerGoing == 0){
       min = preSetMin3;
       sec = preSetSec3;
       setEncodersPos(min, sec);  
     }
}
void pressPresetButton4(){
    if(timerGoing == 0){
       min = preSetMin4;
       sec = preSetSec4;
       setEncodersPos(min, sec);  
    }
}  
 void pressPresetButton5(){
    if(timerGoing == 0){
       min = preSetMin5;
       sec = preSetSec5;
       setEncodersPos(min, sec);  
     }
 }
//-------- Long holds of presets-------------------------------
//these will save the current time to the EEPROM and blink to let you know it's saving
  void longPressStart(){
    if(timerGoing == 0){
       iochip.digitalWrite(ledDone, LOW);  
    }
 }
 
 void longPressStopPresetButton1(){
    if(timerGoing == 0){
      EEPROM.update(10, min);
      EEPROM.update(15, sec);
      preSetMin1 = min;
      preSetSec1 = sec;
      iochip.digitalWrite(ledDone, HIGH);
      delay(75); 
      iochip.digitalWrite(ledDone, LOW);
      delay(75);
      iochip.digitalWrite(ledDone, HIGH); 
    }
 }
void longPressStopPresetButton2(){
  
   if(timerGoing == 0){
      EEPROM.update(20, min);
      EEPROM.update(25, sec);
      preSetMin2 = min;
      preSetSec2 = sec;
     iochip.digitalWrite(ledDone, HIGH);
      delay(75); 
      iochip.digitalWrite(ledDone, LOW);
      delay(75);
      iochip.digitalWrite(ledDone, HIGH);  
    }
}
void longPressStopPresetButton3(){
  if(timerGoing == 0){
      EEPROM.update(30, min);
      EEPROM.update(35, sec);
      preSetMin3 = min;
      preSetSec3 = sec;
     iochip.digitalWrite(ledDone, HIGH);
      delay(75); 
      iochip.digitalWrite(ledDone, LOW);
      delay(75);
      iochip.digitalWrite(ledDone, HIGH); 
  }
}
void longPressStopPresetButton4(){
   if(timerGoing == 0){
      EEPROM.update(40, min);
      EEPROM.update(45, sec);
      preSetMin4 = min;
      preSetSec4 = sec;
       iochip.digitalWrite(ledDone, HIGH);
      delay(75); 
      iochip.digitalWrite(ledDone, LOW);
      delay(75);
      iochip.digitalWrite(ledDone, HIGH);  
    }
}
void longPressStopPresetButton5(){
   if(timerGoing == 0){
      EEPROM.update(50, min);
      EEPROM.update(55, sec);
      preSetMin5 = min;
      preSetSec5 = sec;
       iochip.digitalWrite(ledDone, HIGH);
      delay(75); 
      iochip.digitalWrite(ledDone, LOW);
      delay(75);
      iochip.digitalWrite(ledDone, HIGH);  
   }
}

//-------- updatePresetEEPROM -------------------------------
//Will check the decoVersion against what is saved in the EEPROM 
// and if it does not match update the EEPROM with defaults, if it does,
//it will load the values stored in the EEPROM.
void updatePresetEEPROM(){
    if(decoVersionSaved == decoVersion){     //has been written before
        //load preset times from EEPROM 
        preSetMin1 = EEPROM.read(10);
        preSetSec1 = EEPROM.read(15);
        preSetMin2 = EEPROM.read(20);
        preSetSec2 = EEPROM.read(25);
        preSetMin3 = EEPROM.read(30);
        preSetSec3 = EEPROM.read(35);
        preSetMin4 = EEPROM.read(40);
        preSetSec4 = EEPROM.read(45);
        preSetMin5 = EEPROM.read(50);
        preSetSec5 = EEPROM.read(55);
    }
    else{                                 //has not
        EEPROM.update(10, preSetMin1);
        EEPROM.update(15, preSetSec1);
        EEPROM.update(20, preSetMin2);
        EEPROM.update(25, preSetSec2);
        EEPROM.update(30, preSetMin3);
        EEPROM.update(35, preSetSec3);
        EEPROM.update(40, preSetMin4);
        EEPROM.update(45, preSetSec4);
        EEPROM.update(50, preSetMin5);
        EEPROM.update(55, preSetSec5);
        EEPROM.update(versionAddress, decoVersion);
    }
}

