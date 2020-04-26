// PIVspeed 1.1 by Matthias Doerfler and Maximilian Pfaff
// based on: https://youtu.be/iMvzBdijVwU
// Laser barrier using Arduino Nano
// developed at the University of Freiburg
// for use in the PIV-Lab @ Geology
// source code and manual at https://thegermangeologist.rocks/software/arduino-laser-barrier/

// initialize LCD
// you might need to adjust this for a differnt LCD
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4 ,5 , 6 , 7 , 3, POSITIVE);

// initialize parameters and measurement variables
float distance = 47.0;  // [mm], adjust this according to setup
volatile unsigned long time1 = 0;
volatile unsigned long time2 = 0;
volatile unsigned long T = 0;
float Vms = 0;
float Vcm = 0;
float Vmm = 0;
float Vkmh = 0;

// initialize program states
volatile int trigger1 = 0;
volatile int trigger2 = 0;
int debugactive = 0;
int debugconnected = 0;
int debugmode = 0;
char modeinput = '0';
int modeselected = 0;
int operationalprepared = 0;
int measured = 0;
int reset = 0;
int errorcaught = 0;
int debuginitialized = 0;
int resultsdisplayed = 0;

// name Pins
// LED
#define BLUE 6
#define RED 5
#define GREEN 9
// switches
#define DEBUG 8
#define RESET 7
// laser barriers
#define LBENTRY 2
#define LBEXIT 3

// function prototyping
void checkdebug();
void checkreset();
void resetfcn();
void operationalroutines();
void operating();
void operationalsetup();
void measuring();
void dispresults();
void pentry();
void pexit();
void debugroutines();
void debugframework();
void debugsetup();
void debugstart();
void debugwait();
void modeselection();
void debugmeasuring();
void debugraw();

void setup() {
	// set pin modes
	// laser barrier
	pinMode(2, INPUT);
	pinMode(3, INPUT);
	// debug switch
	pinMode(DEBUG, INPUT);
	// attach interrupts to barriers
	attachInterrupt(digitalPinToInterrupt(LBENTRY),pentry,FALLING);
	attachInterrupt(digitalPinToInterrupt(LBEXIT),pexit,FALLING);
	// initialize serial com for debug mode
	Serial.begin(9600);
	// LED setup
	analogWrite(BLUE,255);
	analogWrite(GREEN,0);
	analogWrite(RED,0);
	// show welcome message
	lcd.begin (20,4);
	lcd.setBacklightPin(3,POSITIVE);
	lcd.setBacklight(HIGH);
	lcd.home();
	lcd.setCursor(0,0);
	lcd.print("Welcome to PIVspeed!");
	lcd.setCursor(0,1);
	lcd.print("Version 1.1, by:");
	lcd.setCursor(0,2);
	lcd.print("Matthias Doerfler");
	lcd.setCursor(0,3);
	lcd.print("Maximilian Pfaff");
	delay(7000);
}

void loop() {
	// check if debugmode is active
	checkdebug();
	// check if we had a reset
	checkreset();
	// reset if above was true
	resetfcn();
	// perform operational routines if not debugging
	operationalroutines();
	// perform debugging routines instead if active
	debugroutines();
}

void checkdebug() {
  // check if we're in debug mode and set state accoringly
	if (digitalRead(DEBUG) == HIGH){
		debugactive = 1;
	}
	else {
		debugactive = 0;
	}
}

void checkreset() {
  // check if reset is triggered
	if (digitalRead(RESET) == HIGH){
		reset = 1;
	}
	else {
		reset = 0;
	}
}

void resetfcn() {
  // reset if desired
	if (reset == 1) {
		// reset all states
		trigger1 = 0;
		trigger2 = 0;
		measured = 0;
		errorcaught = 0;
		debugactive = 0;
		debugmode = 0;
		debugconnected = 0;
		debuginitialized = 0;
		operationalprepared = 0;
		resultsdisplayed = 0;
		// clear LCD
		lcd.clear();
	}
}

void operationalroutines() {
  // only do this if debug inactive
	if (debugactive == 0){
    // cancel if we had an error
    if (errorcaught == 1){
      return;
    }
    // check if we need to prepare for operational
		if (operationalprepared == 1){
			operating();
		}
		else {
			operationalsetup();
		}
	}
}

void operating() {
  // display results if measured, ohterwise wait for measurement
	if (measured == 1){
		dispresults();
	}
	else {
		measuring();
	}
}

void operationalsetup() {
	// set LED status to ready (green)
	analogWrite(BLUE,0);
	analogWrite(GREEN,255);
	analogWrite(RED,0);
	// set LCD text to "ready to shoot"
	lcd.clear();
  lcd.print("Ready to shoot.");
  // update state
  operationalprepared = 1;
}

void measuring() {
	// check if both barriers have been triggered
	if (trigger1 == 1 && trigger2 == 1){
		// check if setup was correct
		if (time2 < time1){
			// display error message
			lcd.clear();
			lcd.print("ERROR");
			lcd.setCursor(0,1);
			lcd.print("Wrong way.");
			lcd.setCursor(0,3);
			lcd.print("Press reset.");
			// set error state to true
			errorcaught = 1;
			// update LED: red
			analogWrite(BLUE,0);
			analogWrite(GREEN,0);
			analogWrite(RED,255);
			return;
		}
		if (time2 > time1){
			// do calculations
			T = time2 - time1;
			Vms = 1000.0 * distance / T;
			Vcm = Vms * 100;
			Vmm = Vcm * 10;
			Vkmh = Vms * 3.6;
			// set measured state to true
			measured = 1;
			return;
		}
	}
}

void dispresults() {
	if (resultsdisplayed == 1){
		return;
	}
	else {
		lcd.clear();
		lcd.print("Speed: ");
		lcd.print(Vms);
		lcd.print(" [m/s]");
		lcd.setCursor(0,1);
		lcd.print("Time: ");
		lcd.print(T);
		lcd.print(" [us]");
		lcd.setCursor(0,3);
		lcd.print("Press reset to clear");
		// set state to results displayed
		resultsdisplayed = 1;
		// update LED: yellow
		analogWrite(BLUE,0);
		analogWrite(GREEN,180);
		analogWrite(RED,255);
	}

}

// functions triggered by interrupt Pins (Laser Barrier)
void pentry() {
	time1 = micros();
	trigger1 = 1;
}

void pexit() {
	time2 = micros();
	trigger2 = 1;
}

void debugroutines() {
	// only do this if debug mode is active
	if (debugactive == 1){
		// check if connection to PC has been established
		if (debugconnected == 1){
			debugframework();
		}
		else {
			debugsetup();
		}
	}
}

void debugframework() {
	// check if a mode has been selected
	if (modeselected == 1){
		// call apropriate function for selected mode
		switch (debugmode) {
		case 1:
			debugmeasuring();
			break;
		case 2:
			debugraw();
			break;
		default:
		break;
	  }
	}
	// ohterwise try to get mode from serial
	else {
		modeselection();
	}
}

void debugsetup() {
	// check if LCD is already updated
	if (debuginitialized == 1){
		debugwait();
	}
	else {
		debugstart();
	}
}

void debugstart() {
	// display instructions on LCD
	lcd.clear();
	lcd.print("DEBUG-MODE");
	lcd.setCursor(0,1);
	lcd.print("Please connect to PC");
	lcd.setCursor(0,2);
	lcd.print("Consult the manual");
	lcd.setCursor(0,3);
	lcd.print("Baud rate 9600");
	// set state to debug initialized true
	debuginitialized = 1;
	// update LED: purple
	analogWrite(BLUE,180);
	analogWrite(GREEN,0);
	analogWrite(RED,255);
}

void debugwait() {
	// check if serial connection to
	// PC with Arduino IDE is established
	if (Serial.available()){
		lcd.clear();
		lcd.print("DEBUG-MODE");
		lcd.setCursor(0,2);
		lcd.print("PC connected.");
		Serial.println("Choose a debug tool:");
		Serial.println("1 to display the measurements here");
		Serial.println("2 to display raw digial values from the sensors");
		// set state to connected true
		debugconnected = 1;
	}
	else {
		// otherwise exit the function
		return;
	}
}

void modeselection() {
	// get selected mode from Serial interface
	modeinput = Serial.read();
	if (modeinput == '1'){
		debugmode = 1;
		modeselected = 1;
	}
	else if (modeinput == '2'){
		debugmode = 2;
		modeselected = 1;
	}
	else {
		// no input from serial found
		modeinput = '0';
		debugmode = 0;
		modeselected = 0;
	}
}

void debugmeasuring() {
	if (trigger1 == 1 && trigger2 == 1){
		// check if setup was correct
		if (time2 < time1){
			// display error message
			Serial.println("ERROR");
			Serial.println("Wrong way.");
			trigger1 = 0;
			trigger2 = 0;
			time1 = 0;
			time2 = 0;
			return;
		}
		if (time2 > time1){
			// do calculations
			T = time2 - time1;
			Vms = 1000.0 * distance / T;
			Vcm = Vms * 100;
			Vmm = Vcm * 10;
			Vkmh = Vms * 3.6;
			Serial.println("Speed:");
			Serial.print(Vms);
      Serial.print("m/s ");
			Serial.print(Vcm);
      Serial.print("cm/s ");
			Serial.print(Vmm);
      Serial.print("mm/s ");
			Serial.print(Vkmh);
      Serial.println("km/h ");
			trigger1 = 0;
			trigger2 = 0;
			time1 = 0;
			time2 = 0;
			return;
		}
	}
}

void debugraw() {
	// just dump the digital readings into serial monitor
  Serial.print("Entry: ");
	Serial.println(digitalRead(LBENTRY));
  Serial.print("Exit: ");
	Serial.println(digitalRead(LBEXIT));
  delay(100);
}
