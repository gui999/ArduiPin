/*
       Name:  ArduiPin
   Revision:  0.2.1a (Alpha)
     Genius:  Guillaume Duplain
Description:  PLEASE READ THE README.txt	 
*/

// *** I/O CONFIG ****  CHANGE THIS TO REPRESENT PIN USED BY YOUR ARDUNINO
byte _inputDigitalPin[]  = {52,53,54,55,56,57,58};           //The array of digital input pin used (buttons).
byte _inputAnalogPin[]   = {6,7};                            //The array of analog input pin used (plunger, nudging, etc).
byte _outputDigitalPin[] = {45,46,47,48,49,50,51};           //The array of digital output pin used.
byte _outputAnalogPin[]  = {9,10};                           //The array of analog output pin used (PWM, RGB, etc).


// *** CONFIG CONSTS ***
const int         BAUD_RATE = 9600;  //Default value (needs to match the PC interface baud rate).
const byte DEBOUNCING_DELAY = 10;    //Delay in ms.
const byte        MSG_START = 2;     //The byte that represents the ASCII "Start of text" char. Used as a "new message" delimiter.. since we start a new text! (there's a concept there)
const byte  CARRIAGE_RETURN = 13;    //The byte that represents the ASCII "carriage return" char.
const byte        HANDSHAKE = 255;   //The byte that define a handshake with the PC interface.
const byte MSG_PARAM_LENGTH = 5;     //The max length of the param zone of a message. Can be changed if using more than 2 shift registers in an array.
const long      MSG_TIMEOUT = 50;    //The delay (ms) the message as to enter the serial buffer before timeout.

const byte INPUT_DIGITAL_COUNT  = sizeof(_inputDigitalPin);  //The number of digital input.
const byte INPUT_ANALOG_COUNT   = sizeof(_inputAnalogPin);   //The number of analog input.
const byte OUTPUT_DIGITAL_COUNT = sizeof(_outputDigitalPin); //The number of digital input.
const byte OUTPUT_ANALOG_COUNT  = sizeof(_outputAnalogPin);  //The number of analog input.


// *** ENUMS ***
typedef enum cmdEnum {INPUT_CMD = 73, PIN_WRITE_CMD = 80, SHIFT_OUT_CMD = 83, PWM_WRITE_CMD = 87}; //I-P-S-W -> Used for the commands sent/received thru the serial link.
typedef enum shiftOutParamEnum {CLOCK_PIN, DATA_PIN, LATCH_PIN, BYTES_TO_SHIFT};                   //Indexes of param used for the shiftOut command.
typedef enum ioParamEnum {PIN, STATE};                                                             //Indexes of param used for normal single I/O command.
typedef enum msgEnum {START_BYTE, CMD_BYTE, PARAM_BYTE};                                           //Indexes for the message structure.


// *** STURCTURES ***
//Button definition
typedef struct Button{  
  byte pin;          	            //The pin #.
  unsigned long lastDebounceMillis; //To use with millis() for software debouncing purpose.
  bool currentState; 	            //The current state of the button.
  bool lastState;    	            //The last state of the button.
  
  Button() : pin(0), lastDebounceMillis(0), currentState(HIGH), lastState(HIGH) {} //Default values (constructor).
} Button;

//Message definition.
//The paramByte is the parameters used for a command. For normal I/O it should use only 2 bytes: pin# & State (HIGH/LOW).
//But it can use more, like PWM pin or when passing bytes to an array for shift register. If you use more shift register, you can change 
//the default value of MSG_PARAM_LENGTH
typedef struct Message{ 
  byte startByte;                   //The start byte = MSG_START.
  byte cmdByte;                     //The command.
  byte paramByte[MSG_PARAM_LENGTH]; //The parameters of the command.
  
  Message() : startByte(MSG_START), cmdByte(0), paramByte() {} //Default values (constructor).
} Message;

const byte MSG_LENGTH = sizeof(Message);  

// *** WORKING VARIABLES ***
Button _button[INPUT_DIGITAL_COUNT];  //The array of Button type struct.

// ************************************** SETUP SECTION ***************************************************
void setup(){
  //IO initialization.
  setInput();
  setOutput();
  
  // Start the serial connection and wait for the handshake
  Serial.begin(BAUD_RATE);
  Serial.setTimeout(MSG_TIMEOUT);
  waitHandShake();
}

//Function that initialize the inputs/buttons used.
void setInput(){
  byte i;
  	
  for (i = 0; i < INPUT_DIGITAL_COUNT; i++) 
  {
    _button[i].pin = _inputDigitalPin[i];
    _button[i].lastDebounceMillis = millis();	
    pinMode(_inputDigitalPin[i], INPUT);
    digitalWrite(_inputDigitalPin[i], HIGH);
  }
}

//Function that initialize the outputs used.
void setOutput(){
  byte i;
  	
  for (i = 0; i < OUTPUT_DIGITAL_COUNT; i++) 
  {	
    pinMode(_outputDigitalPin[i], OUTPUT);
    digitalWrite(_outputDigitalPin[i], LOW);
  }
  	
  for (i = 0; i < OUTPUT_ANALOG_COUNT; i++) 
  {	
    pinMode(_outputAnalogPin[i], OUTPUT);
    digitalWrite(_outputDigitalPin[i], LOW);
  }
  	
  //initialize the internal led pin as an output, for debug purpose.
  pinMode(13, OUTPUT);     
  digitalWrite(13, LOW);
}

//Function that waits for PC handshake (connection established).
//The Arduino is the only one waiting for the handshake since it should always be ON
//as long as the PC is running.
void waitHandShake(){
  bool isHandShakeReceived = false;
  byte data;
  	
  //Waiting for the handshake from the PC
  while (!isHandShakeReceived)
  {
    if (Serial.available())
    {
      data = Serial.read();
      
      //Once we have received it, we exit the handshake loop.
      if (data == HANDSHAKE)
        isHandShakeReceived = true;			
    }
  }
}
// ************************************** SETUP SECTION END ***********************************************

// ************************************** MAIN LOOP SECTION ***********************************************

//The main routine that never ends.. ever.. even if you close the PC it will still run as long it as power!
//We enter it as soon as we received the handshake from the PC
void loop(){
  Message msg;
  	
  readInput(); //We read the inputs/buttons first.
  	
  //If a message is ready to be processed, we process it.
  if (readMessage(msg))
    processMessage(msg);
}


//Function that read the inputs/buttons and sends the data thru the serial port.
//The way it works, we send data 2 bytes at a time in this way: Pin# and it's state
//Example: we push on the left flipper which is pin 52. It would send: 520 -> the byte 52(pin) and the byte 0(it's state)
void readInput(){
  byte i;
  
  //We loop to check the state of each button in the array.
  for (i = 0; i < INPUT_DIGITAL_COUNT; i++) 
  {
    _button[i].currentState = digitalRead(_button[i].pin); //Read the pin state
    	
    //If the state of the button changed and we have reached the deboucing delay.
    if ((_button[i].currentState != _button[i].lastState) && (millis() - _button[i].lastDebounceMillis > DEBOUNCING_DELAY))
    { 	 
      digitalWrite(13, !_button[i].currentState); //For debugging purpose
    		
      _button[i].lastState = _button[i].currentState; //Save the state.
      _button[i].lastDebounceMillis = millis(); //Save the time for debouncing.
    
      //We prepare a new message to be send.
      struct Message msg;
      msg.cmdByte = INPUT_CMD;
      msg.paramByte[PIN]   = _button[i].pin;
      msg.paramByte[STATE] = _button[i].currentState;

      sendInput(msg); //Finaly we send the message to the PC interface.  
    }
  }
}

//Function that reads the serial port buffer until it hits the carriage return delimiter and 
//reconstruct the received message. The message is passed by reference so it's value can be changed
//before returning back into the main loop.
//Return: bool-> TRUE if message is OK, false if message is not OK.
bool readMessage(struct Message &msg){
  byte data[MSG_LENGTH+1]; //+1 for the delimiter char which is returned by readBytesUntil()
  bool isMessageOK = false;
  int byteRead;
  	
  //If we have data in the buffer, we read it until we hit the carriage return delimiter
  //or hit the timeout delay, or hit the max lenght permited (lenght of a message).
  if (Serial.available()){	
    byteRead = Serial.readBytesUntil(CARRIAGE_RETURN, (char *)data, MSG_LENGTH+1);
    //If the lenght and the structure of the byte read is OK, we reconstruct the message.
    if (byteRead == MSG_LENGTH && data[START_BYTE] == MSG_START && data[MSG_LENGTH - 1] == CARRIAGE_RETURN){
      isMessageOK = true;
      byte i;
      msg.cmdByte = data[CMD_BYTE];
      		
      for (i = 0; i < MSG_PARAM_LENGTH; i++){		
        msg.paramByte[i] = data[PARAM_BYTE + i];
      }
    }
  }
  
  return isMessageOK;
}

//Function that process the message (if it's ready) depending on it's command. 
void processMessage(struct Message &msg){
  switch (msg.cmdByte){
    case PIN_WRITE_CMD:
          pinWriteCmd(msg);
          break;
    case SHIFT_OUT_CMD:
          shiftOutCmd(msg);
          break;
    case PWM_WRITE_CMD:
          pwmWriteCmd(msg);
          break;	
    default: //If the command is not recognized.. well we do nothing.
          break;	
  }
}

//Function that write to a single output digital pin. Should not be used that often since the Arduino
//needs shift registers and transistor/arrays to run outputs > 30ma. 
void pinWriteCmd(struct Message &msg){
  //If the pin is not null and the state is LOW/HIGH (0/1)
  if (msg.paramByte[PIN] != 0 && msg.paramByte[STATE] < 2)
    digitalWrite(msg.paramByte[PIN], msg.paramByte[STATE]);		
}

//Function that write to several pins (using shift registers). Should be used as the main output function.
void shiftOutCmd(struct Message &msg){
  byte i;
  	
  digitalWrite(msg.paramByte[LATCH_PIN], LOW);
  	
  for (i = BYTES_TO_SHIFT; i < MSG_PARAM_LENGTH; i++){
    shiftOut(msg.paramByte[DATA_PIN], msg.paramByte[CLOCK_PIN], MSBFIRST, msg.paramByte[BYTES_TO_SHIFT + i]);
  }
  	
  digitalWrite(msg.paramByte[LATCH_PIN], HIGH);
}

//Function that write to a PWM analog pin for motor/shaker control.
void pwmWriteCmd(struct Message &msg){
  if (msg.paramByte[PIN] != 0)
    analogWrite(msg.paramByte[PIN], msg.paramByte[STATE]);
}

//Function that send data to the PC interface.
//We use Serial.write() instead of println() since we want to send the actual byte of the message (not the chars).
//println() is used at the end as the message delimiter.
void sendInput(struct Message msg){
  Serial.write(msg.startByte);
  Serial.write(msg.cmdByte);
  Serial.write(msg.paramByte, MSG_PARAM_LENGTH);
  Serial.println();
}
