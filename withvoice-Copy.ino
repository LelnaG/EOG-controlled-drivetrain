
#include <TimerOne.h>                          
#define trigPin 12                                    // Pin 12 trigger output
#define echoPin 2                                     // Pin 2 Echo input
#define onBoardLED 13                                 // Pin 13 onboard LED
#define echo_int 0                                    // Interrupt id for echo pulse

#define TIMER_US 50                                   // 50 uS timer duration 
#define TICK_COUNTS 4000                              // 200 mS worth of timer ticks

#define voice 7
#define voiceValue LOW 


volatile long echo_start = 0;                         // Records start of echo pulse 
volatile long echo_end = 0;                           // Records end of echo pulse
volatile long echo_duration = 0;                      // Duration - difference between end and start
volatile int trigger_time_count = 0;                  // Count down counter to trigger pulse time
volatile long range_flasher_counter = 0;  
// the setup routine runs once when you press reset:

const int motorPin1  = 5;  // Pin 14 of L293
const int motorPin2  = 4;  // Pin 10 of L293
//Motor B
const int motorPin3  = 10; // Pin  7 of L293
const int motorPin4  = 9;  // Pin  2 of L293
const int enA=6;
const int enB=11;
int emergencyDistance_State = 0;
int emergencyDistance = 20;
int emergencyAngle_State = 0;

//voice control

static float difference;


void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);                           // Trigger pin set to output
  pinMode(echoPin, INPUT);                            // Echo pin set to input
  pinMode(onBoardLED, OUTPUT);   
 
    pinMode(motorPin1, OUTPUT);
    pinMode(motorPin2, OUTPUT);
    pinMode(motorPin3, OUTPUT);
    pinMode(motorPin4, OUTPUT);
    pinMode(enA,OUTPUT);
    pinMode(enB,OUTPUT);
   
//    analogWrite(enA,160);
//    analogWrite(enB,160);
    
calibration();
delay(100);
  Timer1.initialize(TIMER_US);                        // Initialise timer 1
  Timer1.attachInterrupt(timerIsr);                 // Attach interrupt to the timer service routine 
  attachInterrupt(echo_int, echo_interrupt, CHANGE);  // Attach interrupt to the sensor echo input     
  
}

// the loop routine runs over and over again forever:
void loop() {
  boolean voiceState = voiceValue;
  voiceState = digitalRead(voice);
  //First "LOD": Voice control on --> "Go"
  if(voiceState==HIGH){
    Serial.println(echo_duration / 58);               // Print the distance in centimeters
    delay(100);

    //Second LOD: clear driving pathway; no obstacles  
    if (emergencyDistance_State == 0){
      digitalWrite(onBoardLED, HIGH);
      int pinWiper= analogRead(A1);
      float volts= (pinWiper*5.0)/1023.0;
      
     
      //looking straight --> 
      if(((volts+difference)<2.55)&&((volts+difference)>=2.45)){// && digitalRead(7)==HIGH && emergencyDistance_State == 0 ){
        Serial.println("looking straight");
      
        analogWrite(enA,130);
        
        digitalWrite(motorPin1, HIGH);
        digitalWrite(motorPin2, LOW);
        
        analogWrite(enB,130);
        digitalWrite(motorPin3, HIGH);
        digitalWrite(motorPin4, LOW);
        
//        pinWiper= analogRead(A1);
//        delay(10); //sampling at 100Hz
//        volts= (pinWiper*5.0)/1023.0; 
      }
    else if ((volts+difference)>=2.55){ // && digitalRead(7)==HIGH && emergencyDistance_State == 0){
      Serial.println("looking right"); 
    
      analogWrite(enB,130);
      digitalWrite(motorPin3, HIGH);
      digitalWrite(motorPin4, LOW);
      analogWrite(enA,0);
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, LOW);
        
//     pinWiper= analogRead(A1); 
//     delay(10);
//     volts= (pinWiper*5.0)/1023.0; 
      }
    else if ((volts+difference)<2.45){ //&& digitalRead(7)==HIGH && emergencyDistance_State == 0){
      Serial.println("looking left"); 
    
      analogWrite(enB,0);
      digitalWrite(motorPin3, LOW);
      digitalWrite(motorPin4, LOW);
      analogWrite(enA,130);
      digitalWrite(motorPin1, HIGH);
      digitalWrite(motorPin2, LOW);
        
//      pinWiper= analogRead(A1); 
//      delay(10);
//      volts= (pinWiper*5.0)/1023.0; 
     }
    }
    
    else if (emergencyDistance_State == 1){

      digitalWrite(onBoardLED, LOW);
      analogWrite(enA, 0);
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, LOW);
      
      analogWrite(enB, 0);
      digitalWrite(motorPin3, LOW);
      digitalWrite(motorPin4, LOW);
  
      delay(1000);
      //LCD Display: Obstacle Detected. 
      //LCD Display: Reversing in 5 seconds.
  
      analogWrite(enA,130);
    
      digitalWrite(motorPin1, LOW);
      digitalWrite(motorPin2, HIGH);
      
      analogWrite(enB,130);
      digitalWrite(motorPin3, LOW);
      digitalWrite(motorPin4, HIGH);
  
      delay(2000);
      

    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);
    
    analogWrite(enB, 0);
    digitalWrite(motorPin3, LOW);
    digitalWrite(motorPin4, LOW);

    delay(3000);
    //LCD: move eyes now
      
  
      voiceState = voiceValue;
     
    }
}
else if(voiceState==LOW){
    analogWrite(enA, 0);
    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);
    
    analogWrite(enB, 0);
    digitalWrite(motorPin3, LOW);
    digitalWrite(motorPin4, LOW);
  
  
  }
}
      

void timerIsr()
{
    trigger_pulse();                                 // Schedule the trigger pulses
    emergencyState_Distance();                       // Change the state of the distance emergency;
}

void trigger_pulse()
{
      static volatile int state = 0;                 // State machine variable

      if (!(--trigger_time_count))                   // Count to 200mS
      {                                              // Time out - Initiate trigger pulse
         trigger_time_count = TICK_COUNTS;           // Reload
         state = 1;                                  // Changing to state 1 initiates a pulse
      }
    
      switch(state)                                  // State machine handles delivery of trigger pulse
      {
        case 0:                                      // Normal state does nothing
            break;
        
        case 1:                                      // Initiate pulse
           digitalWrite(trigPin, HIGH);              // Set the trigger output high
           state = 2;                                // and set state to 2
           break;
        
        case 2:                                      // Complete the pulse
        default:      
           digitalWrite(trigPin, LOW);               // Set the trigger output low
           state = 0;                                // and return state to normal 0
           break;
     }
}

void echo_interrupt()
{
  switch (digitalRead(echoPin))                     // Test to see if the signal is high or low
  {
    case HIGH:                                      // High so must be the start of the echo pulse
      echo_end = 0;                                 // Clear the end time
      echo_start = micros();                        // Save the start time
      break;
      
    case LOW:                                       // Low so must be the end of hte echo pulse
      echo_end = micros();                          // Save the end time
      echo_duration = echo_end - echo_start;        // Calculate the pulse duration
      break;
  }
}


void emergencyState_Distance(){
  if (--range_flasher_counter <=0){
    int distance = echo_duration * 0.034/2;
    if (distance > emergencyDistance){
      emergencyDistance_State = 0;
      //LCD Screen: "Ready"
    }
    else{
      emergencyDistance_State = 1;
      //LCD Screen: "ERROR: Obstruction Detected"
      }
    }
    
  }
   
void calibration(){
  Serial.println("starting calibration, please look straight for 2 seconds");
  delay(2000);
     int pinWiper= analogRead(A1); 
    float volts= (pinWiper*5.0)/1023.0;
   difference=2.5-volts;
   Serial.println("calibration complete!");
   delay(100);
  }
