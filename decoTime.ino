/*********************************************************************************************
 Project: Deco Tea timer
 By: Andy Evans 
 Date: 8/12/2015
 This is a retor tea timer that usese speedometer steppers (switecx25) to show the time. 
 It uses two rotery encoders with push buttons to set the time, and a number
 of leds to show the percent left. When finished it will rinf a bell and 
 flash an LED. It does this all via a MCP23S17 chip.
 INCLUDES: 
 -MCP23S17 - http://playground.arduino.cc/Main/MCP23S17
 -UP/Down Timer - http://playground.arduino.cc/Main/CountUpDownTimer
 -switecX25 - https://github.com/clearwater/SwitecX25
 -SPI - https://www.arduino.cc/en/Reference/SPI
***********************************************************************************************/

#include "SwitecX25.h"
#include "CountUpDownTimer.h"
#include <SPI.h>              // We use this library, so it must be called here.
#include <MCP23S17.h>         // Here is the new class to make using the MCP23S17 easy.


//Rotary Encoder Max/Min
int maximum = 255;
int minimum = 0;

int encoderMinPinA = 2;  //ENCODER1
int encoderMinPinB = 1;  //ENCODER1
int encoderSecPinA = 3;  //ENCODER2
int encoderSecPinB = 4;  //ENCODER2

int startStopPin = 5;
int resetTimerPin = 6;
int ledDone = 7;
int bellPin = 8;
int led1 = 9;
int led2 = 10;
int led3 = 11;
int led4 = 12;
int led5 = 13;
int led6 = 14;
int led7 = 15;
int led8 = 16;
long ledPreviousMillis = 0;        // will store last time LED was updated
long ledInterval = 400;
int ledDoneState = LOW;

//preset buttons, using analog pins, and a digital. Ideally one could set up all 5 buttons on one alalog by using resistors. 
char presetButton1pin = A1;  //analog 1
char presetButton2pin = A2;  //analog 2
char presetButton3pin = A3;  //analog 3
char presetButton4pin = 0;  //digital 0, needed the analog 4 for SDA on expantion port
char presetButton5pin = 1;  //digital 1, needed the analog 5 for SCL on expantion port

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

//================================ SETUP ===================================================================  
void setup() {
  
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
  pinMode(presetButton1pin, INPUT_PULLUP);
  pinMode(presetButton2pin, INPUT_PULLUP);
  pinMode(presetButton3pin, INPUT_PULLUP);
  pinMode(presetButton4pin, INPUT_PULLUP);
  pinMode(presetButton5pin, INPUT_PULLUP);
  
  Serial.begin (115200);
  
  // run both motors against stops to re-zero
  motor1.zero();   // this is a slow, blocking operation
  motor2.zero();
  
  motor1.setPosition(0);
  motor2.setPosition(0);
}

//++++++++++++++++++++++++++++++++++++ LOOP +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
void loop() {
  // update motors frequently to allow them to step
  motor1.update();
  motor2.update();
 
//=========================== timer logic ===================================================================   
  if(timerGoing==1)
  {
    theClock.Timer(); // run the timer
    if (theClock.TimeHasChanged() ) // this prevents the time from being constantly shown.
        {
          motor1.setPosition(abs(15.75*theClock.ShowSeconds()));
          // motor2.setPosition(900);
          motor2.setPosition(60*theClock.ShowMinutes());
          //motor1.updateBlocking();
          //motor2.updateBlocking();
          
          currentTotalSec = ((int)theClock.ShowMinutes()*60) + (int)theClock.ShowSeconds();
          
          Serial.print(theClock.ShowHours());
          Serial.print(":");
          Serial.print(theClock.ShowMinutes());
          Serial.print(":");
          Serial.print(theClock.ShowSeconds());
          Serial.print(":");
          Serial.println(theClock.ShowMilliSeconds());
          // This DOES NOT format the time to 0:0x when seconds is less than 10.
          // if you need to format the time to standard format, use the sprintf() function.
        }
  }
  else
  {
    sec = encoderSecPos;
    min = encoderMinPos/17;
    motor1.setPosition(abs(15.75*sec));
    motor2.setPosition(60*min);
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
    Serial.println ((String)encoderMinPos);
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
    Serial.println (encoderSecPos);
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
 

 if(digitalRead(presetButton1pin) == LOW){
   min = 0;
   sec = 30;
   Serial.println("Button 1 press");
 }
 if(digitalRead(presetButton2pin)==LOW){
   min = 2;
   sec = 0;
   Serial.println("Button 2 press");
 }
 if(digitalRead(presetButton3pin)==LOW){
   min = 3;
   sec = 30;
   Serial.println("Button 3 press");
 }
 if(digitalRead(presetButton4pin)==LOW){
   min = 5;
   sec = 0;
   Serial.println("Button 4 press");
 }
 if(digitalRead(presetButton5pin)==LOW){
   min = 15;
   sec = 0;
   Serial.println("Button 5 press");
 }
 
 
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
      delay(50);
      iochip.digitalWrite(bellPin, LOW);
      delay(1000);
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

