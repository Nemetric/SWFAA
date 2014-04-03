// INCLUDES  24
#include <XBOXRECV.h>
#include <DualVNH5019MotorShield.h>
#include <SimpleTimer.h>
#include <Servo.h>

#include <Encoder.h>

#include <avr/wdt.h>

//#ifdef dobogusinclude
//#include <spi4teensy3.h>
//#endif

// PRIVATE STATE VARIABLES
float _leftSpeed;
float _rightSpeed; 
float _liftSpeed;
int _intakeDirection;
bool _isThrowing; 

bool _autoRunning;
int _autoProgNum;
int _autoInterval;

int _maxAmps;
int _totalAmps;
long _encLeftPos;
long _encRightPos;
long _encLiftPos;

bool _led01HIGH;
bool _led02HIGH;

bool _useLiftEncoder;
int _liftEncMax = 50;
int _liftEncMin = 0;
int _liftEncGoal = 37;

bool _chinupMode;

// PUBLIC CONSTANT VALUES
// Enable/Disable Serial Output
const bool _serial = true;

// Motor Controller 1 Pinouts 
const int _mc1_INA1 = 26;
const int _mc1_INB1 = 28;
const int _mc1_EN1DIAG1 = 22;
const int _mc1_CS1 = A3;
const int _mc1_INA2 = 24;
const int _mc1_INB2 = 30;
const int _mc1_EN1DIAG2 = 32;
const int _mc1_CS2 = A2; 
const int _mc1_PWM1 = 2;
const int _mc1_PWM2 = 3;

// Motor Controller 2 Pinouts
const int _mc2_INA1 = 27;
const int _mc2_INB1 = 29;
const int _mc2_EN1DIAG1 = 23;
const int _mc2_CS1 = A5;
const int _mc2_INA2 = 25;
const int _mc2_INB2 = 31;
const int _mc2_EN1DIAG2 = 33;
const int _mc2_CS2 = A4; 
const int _mc2_PWM1 = 4;
const int _mc2_PWM2 = 5;

//Encoder Left Pinouts
const int _encLeftInt = 19;
const int _encLeftDig = 39;

//Encoder Right Pinouts
const int _encRightInt = 21;
const int _encRightDig = 35;

//Encoder LIFT Pinouts
const int _encLiftInt = 20;
const int _encLiftDig = 37;

//LED Pinouts
const int _led01 = 41;
const int _led02 = 43;

//RESET Digital Pin
const int _resetPin = 6;

//USB Pinouts
//Are Static in Library
/*
const int _usb1 = 47; //Digital 2
const int _usb1 = 49; //Digital 1
const int _usb1 = 50; //MISO
const int _usb1 = 51; //MOSI
const int _usb1 = 52; //SCK
*/

// Intake motors pinouts
//const int _intake1 = 34;
//const int _intake2 = 35;

bool _controllerConnected;
bool _watchdogInitialized;


// Timer Polling Intervals
const int _readControllerInterval = 10;
const int _writeControllerInterval = 15;
const int _writeRobotInterval = 20;
const int _readRobotInterval = 25;
const int _heartbeatInterval = 750;

// Autonomous Timeout Duration (milliseconds)
const int _maxAutonomousDuration = 60000;

//PUBLIC VARIABLES
USB Usb;
XBOXRECV Xbox(&Usb);
DualVNH5019MotorShield md1(_mc1_INA1, _mc1_INB1, _mc1_EN1DIAG1
	, _mc1_CS1, _mc1_INA2, _mc1_INB2, _mc1_EN1DIAG2, _mc1_CS2, _mc1_PWM1, _mc1_PWM2);
DualVNH5019MotorShield md2(_mc2_INA1, _mc2_INB1, _mc2_EN1DIAG1
	, _mc2_CS1, _mc2_INA2, _mc2_INB2, _mc2_EN1DIAG2, _mc2_CS2, _mc2_PWM1, _mc2_PWM2);

Encoder encLeft(_encLeftInt, _encLeftDig);
Encoder encRight(_encRightInt, _encRightDig);
Encoder encLift(_encLiftInt, _encLiftDig);

//http://playground.arduino.cc/Code/SimpleTimer
SimpleTimer timer;


void setup() 
{
	//have watchdog enabled, reset & disable so that it doesn't loop
	//wdt_reset();
	//wdt_disable();

	digitalWrite(_resetPin, HIGH);
	pinMode(_resetPin, OUTPUT); 

	// assign public state variables
	_leftSpeed = 0.0;
	_rightSpeed = 0.0;
	_liftSpeed = 0.0;
	_intakeDirection = 0;
	_isThrowing = false;

	_autoRunning = false;
	_autoProgNum = 0; 
	_autoInterval = 0;

	_totalAmps = 0;
	_maxAmps = 0;
	_encLeftPos = 0;
	_encRightPos = 0;
	_encLiftPos = 0;

	_led01HIGH = true;
	_led02HIGH = true;

	_controllerConnected = false;
	_watchdogInitialized = false;

	_useLiftEncoder = false;
	_chinupMode = false;

	//intake1.attach(_intake1);
	//intake2.attach(_intake2);

	//Set status LED pins to OUTPUTS and set to On
	pinMode(_led01, OUTPUT);
	pinMode(_led02, OUTPUT);
	digitalWrite(_led01, _led01HIGH);
	digitalWrite(_led01, _led01HIGH);

	if(_serial)
	{
		Serial.begin(115200);
  		while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  	}

	logMsg("Starting Xbox Wireless Receiver Library...");
  	if (Usb.Init() == -1) 
  	{
  		logMsg("USB did not start!");
    	while (1); //halt
	}
	logMsg("Xbox Wireless Receiver Library Started");

	logMsg("Initializing Dual VNH5019 Motor Shield 1..."); 
	md1.init();
	logMsg("Dual VNH5019 Motor Shield 1 Initialized"); 

	logMsg("Initializing Dual VNH5019 Motor Shield 2..."); 
	md2.init();
	logMsg("Dual VNH5019 Motor Shield 2 Initialized"); 

	logMsg("Initializing Timers...");
	timer.setInterval(_readControllerInterval, readController);
	timer.setInterval(_writeControllerInterval, writeController);
	timer.setInterval(_readRobotInterval, readRobot);
	timer.setInterval(_writeRobotInterval, writeRobot);
	timer.setInterval(_heartbeatInterval, heartBeat);
	logMsg("Timers Initialized");

	
	//ResetLift();

}

//This might not work...
void logMsg(String msg)
{
	if(_serial)
	{
		Serial.println("\r\n :: " + msg);
	}
}

//This Doesn't work
//void(* resetFunc) (void) = 0; //declare reset function @ address 0

//This Doesn't work either
//void osftware_Reset()
//{
//	asm volatile ("  jmp 0");
//}

void loop() 
{
	// put your main code here, to run repeatedly: 
	timer.run();
  
}
void heartBeat()
{
	// This gets set to true when the controller intially connects.. No sense in heartbeating if there is not a controller
	// connected in the first place
	if(_controllerConnected)
	{
		if(!_watchdogInitialized)
		{
			//Start the WatchDog!
			//Moved the wdt enable to heartbeat because it needs to wait until the USB inits
			//logMsg("WatchDog Started");
			//wdt_enable (WDTO_1S);
			//wdt_reset();
			_watchdogInitialized = true;
		}
		else
		{
			if ((!Xbox.XboxReceiverConnected || !Xbox.Xbox360Connected[0]))
			{
		  		//_controllerConnected = false;
		      	Serial.println("XBOX DISCONNECTED (heartbeat)");

		      	Serial.println("resetting");
				delay(10);
				digitalWrite(_resetPin, LOW);
		      	//osftware_Reset();
		  		//resetFunc();

		  		
		    }
		    else
		    {
		    	wdt_reset();

				digitalWrite(_led01, _led01HIGH);
				_led01HIGH = !_led01HIGH;
		    	//wdt_reset();
		    	//Serial.println("Heartbeat");

		    	
		    }
		}
	}
}

void readRobot()
{
	_totalAmps = md1.getM1CurrentMilliamps() + md1.getM2CurrentMilliamps();
    _totalAmps += md2.getM1CurrentMilliamps() + md2.getM2CurrentMilliamps();
    
    _totalAmps = _totalAmps / 1000 / 2;




    if(_totalAmps > _maxAmps)
		    	{
		    		_maxAmps = _totalAmps;
		    		
		    	}
    //Serial.println(amps);


	if(_encLeftPos != -1* encLeft.read() || 
		_encRightPos != encRight.read() || 
		(_encLiftPos) != encLift.read() )
	{
		Serial.print("LeftEnc: ");
	    Serial.print(_encLeftPos);
	    Serial.print("\t");
		Serial.print("RightEnc: ");
	    Serial.print(_encRightPos);
	    Serial.print("\t");
		Serial.print("LIFT Enc: ");
	    Serial.print(_encLiftPos);
	    Serial.print("\t");

	    Serial.println();

	    digitalWrite(_led02, _led02HIGH);
		_led02HIGH = !_led02HIGH;
	}

    //Read Encoers and update position
    _encLeftPos = -1* encLeft.read();
	_encRightPos = encRight.read();
	_encLiftPos = encLift.read();
}

void readController()
{
	Usb.Task();

	if (Xbox.XboxReceiverConnected) 
	{
    	for (uint8_t i = 0; i < 4; i++) 
    	{
      		if (Xbox.Xbox360Connected[i]) 
      		{
      			_controllerConnected = true;

      			//Pressing this button will reset lift and toggle into encoder mode.
      			if (Xbox.getButtonClick(B, i)) 
      			{
      				_useLiftEncoder = !_useLiftEncoder;
      				//Reset to zero (makes the assumption that lift is all the way down.)
      				if(_useLiftEncoder)
      				{
      					encLift.write(0);
      				}
      			}

      			if (Xbox.getButtonClick(Y, i)) 
      			{
      				_chinupMode = !_chinupMode;
      			}
      			
      			//Left Hat Y Axis
      			if (Xbox.getAnalogHat(LeftHatY, i) > 7500 || Xbox.getAnalogHat(LeftHatY, i) < -7500) {
      				_leftSpeed = 400.0 / 32767 * Xbox.getAnalogHat(LeftHatY, i); 
      				
      			}
      			else
      			{
      				_leftSpeed = 0.0;
      				
      			}

      			//Right Hat Y Axis
      			if (Xbox.getAnalogHat(RightHatY, i) > 7500 || Xbox.getAnalogHat(RightHatY, i) < -7500) {
      				_rightSpeed = 400.0 / 32767 * Xbox.getAnalogHat(RightHatY, i); 
      			}
      			else
      			{
      				_rightSpeed = 0.0;
      			}

      			//Left Hat Click
      			if (Xbox.getButtonClick(L3, i))
      			{
      				//Throw();
      			}

      			//Right Hat Click
      			if (Xbox.getButtonClick(R3, i))
      			{

      			}

      			//L2 Trigger
        		if (Xbox.getButtonPress(L2, i)) 
        		{
        			_liftSpeed = 400.0 / 255 * Xbox.getButtonPress(L2, i) * -1; 
        		}
        		//R2 Trigger
        		else if (Xbox.getButtonPress(R2, i)) 
				{
					_liftSpeed = 400.0 / 255 * Xbox.getButtonPress(R2, i); 
				}
				else
				{
					_liftSpeed = 0.0;
				}

				//L1 Button
				if (Xbox.getButtonPress(L1, i))
				{
					_intakeDirection = 1;
				}
				//R1 Button
				else if (Xbox.getButtonPress(R1, i))
				{
					_intakeDirection = -1;
				}
				else
				{
					_intakeDirection = 0;
				}

				// Start Button 
				if(Xbox.getButtonClick(START, i))
				{
					if(!_autoRunning) 
					{
						if(_autoProgNum != 4) 
						{
							_autoProgNum++;
						} 
						else 
						{
							_autoProgNum = 1;
						}
					} 
				}

				// Back Button
				if(Xbox.getButtonClick(BACK, i))
				{
					if(!_autoRunning) 
					{
						_autoProgNum = 0;
					}
				}

				// XBox Button
				if(Xbox.getButtonClick(XBOX, i))
				{
					if(_autoProgNum != 0)
					{
						if(!_autoRunning)
						{
							// Start Program
							autoStart();
						} 
						else 
						{
							// Stop Program
							autoStop();
						}
					}
				}

				
      		}
      	}
    }
}

void writeController()
{
	if (Xbox.XboxReceiverConnected) 
	{
    	for (uint8_t i = 0; i < 4; i++) 
    	{
			
			//Make sure were not in auto program mode
			if(_autoProgNum == 0)
			{

				if(_totalAmps > 1 && _totalAmps <= 2)
			    {
			       Xbox.setLedOn(LED1, i);
			    }
			    
			    if(_totalAmps > 2 && _totalAmps <= 3)
			    {
			       Xbox.setLedOn(LED2, i);
			    }
			    
			    if(_totalAmps > 3 && _totalAmps <= 4)
			    {
			       Xbox.setLedOn(LED3, i);
			    }
			    
			    if(_totalAmps > 4)
			    {
			       Xbox.setLedOn(LED4, i);
			    }
			    
			    if(_totalAmps < 1)
			    {
			      Xbox.setLedOn(OFF, i);
			    }
			    
			    uint8_t rmbl = 0;
			    if(_totalAmps > 4)
			    {
			    	rmbl = 255.0 / 6 * (_totalAmps-4);
			    }
			    Xbox.setRumbleOn(0,rmbl,i);
			}
			else
			{
				switch(_autoProgNum)
				{
					case 1: 
						Xbox.setLedOn(LED1, i);
						break;
					case 2:
						Xbox.setLedOn(LED2, i);
						break;
					case 3: 
						Xbox.setLedOn(LED3, i);
						break;
					case 4:
						Xbox.setLedOn(LED4, i);
						break;
				}
			}
		}
	}
}

void writeRobot()
{
	if (!_autoRunning)
	{
		MoveSpeed(_leftSpeed, _rightSpeed);
		LiftSpeed(_liftSpeed);
		MoveIntake(_intakeDirection);
		

	} 
	else 
	{
		switch(_autoProgNum)
		{
			case 1: 
				autoRedGoal();
				break;
			case 2:
				autoBlueGoal();
				break;
			case 3: 
				autoRedChin();
				break;
			case 4:
				autoBlueChin();
				break;
		}
		_autoInterval = _autoInterval + _writeRobotInterval;

		//Safety switch in case forgot to call autoStop
		if(_autoInterval > _maxAutonomousDuration)
		{
			//autoStop();
		}
	}
}

// ===========================================
// AUTONOMOUS METHODS
// ===========================================

// Initialize Autonomous Mode
void autoStop()
{
	_autoRunning = false;
	_autoProgNum = 0;
	_autoInterval = 0;
}

void autoStart()
{
	if(_autoProgNum != 0)
	{
		_autoRunning = true;
		_autoInterval = 0;
	}
}

// XBox Button 4
void autoBlueChin()
{
	switch(_autoInterval)
	{
		case 0:
			MoveSpeed(400, 400);
			break;

		case 2000:
			MoveSpeed(400, -400);
			break;

		case 3500:
			MoveSpeed(400, 400);
			break;

		case 5000:
			MoveSpeed(-400, 400);
			break;

		case 6000:
			MoveSpeed(400, 400);
			break;

		case 7500:
			MoveSpeed(-400, 400);
			break;

		case 8000:
			MoveSpeed(-400, -400);
			break;

		case 10000:
			MoveSpeed(0, 0);
			autoStop();
			break;
			
	}
}
// XBox Button 3
void autoRedChin()
{
	switch(_autoInterval)
	{
		case 0:
			MoveSpeed(200, 200);
			MoveIntake(-1);
			break;

		
		case 2300:
			MoveSpeed(-400, -400);
			MoveIntake(0);
			break;
		

		case 2700:
			MoveSpeed(-400, 400);
			break;

		case 3540:
			MoveSpeed(300, 300);
			LiftSpeed(-200);
			break;

		case 4820:
			LiftSpeed(0);
			MoveIntake(0);
			break;

		case 5820:
			LiftSpeed(300);
			MoveSpeed(400,400);
			MoveIntake(1);
			break;

		case 7440:
			MoveSpeed(0, 0);
			MoveIntake(0);
			LiftSpeed(0);
			autoStop();
			break;
			
	}

}
//XBox Button 2
void autoBlueGoal()
{
	MoveIntake(1);
	delay(1000);
	MoveIntake(-1);
	delay(1000);
	MoveIntake(0);

	autoStop();
}
//XBox Button 1
void autoRedGoal()
{
	switch(_autoInterval)
	{
		case 0:
			MoveSpeed(400,400);
			break;
		case 200:
			MoveSpeed(-400, 400);
			break;
		case 400:
			MoveSpeed(400,400);
			break;
		case 1000:
			MoveSpeed(0, 0);
			autoStop();
			break;


	}
}





// ===========================================
// ROBOT METHODS
// ===========================================

 void MoveSpeed(float leftSpeed, float rightSpeed)
 {
 	

	//Set Right Speed
	md1.setM1Speed(-1 * leftSpeed);

	//Set Left Speed
	md1.setM2Speed(-1 * rightSpeed);
	//Serial.println(leftSpeed);
		//Serial.println(_rightSpeed);

	//if (rightSpeed == 0 && leftSpeed == 0)
	//{
	//	md1.setM2Brake(400);
	//	md1.setM1Brake(400);
	//}
 }

 void MoveTime(float leftSpeed, float rightSpeed, long duration)
 {

 }

 void LiftSpeed(float liftSpeed)
 {
 	//Serial.println(liftSpeed);
 	if(_useLiftEncoder)
 	{
	 	if(_chinupMode)
	 	{
	 		if(_encLiftPos >= _liftEncMax  && liftSpeed > 0)
		 	{
		 		md1.setM1Brake(400);
				md1.setM2Brake(400);
		 	}
		 	else if(_encLiftPos <= (_liftEncMin+5) && liftSpeed < 0)
			{
				md1.setM1Brake(400);
				md1.setM2Brake(400);
			}
			else
			{
			 	//Set Lift Speed, Brake if Zero
				if(liftSpeed > -25 && liftSpeed < 25)
				{
					md2.setM2Brake(400);
				}
				else
				{
					if(_encLiftPos > 40 || _encLiftPos < 5)
					{
						liftSpeed = liftSpeed * .25;
					}
					else if(_encLiftPos > 30 || _encLiftPos < 10)
					{
						liftSpeed = liftSpeed * .50;
					}
					else if(_encLiftPos > 20 || _encLiftPos < 15)
					{
						liftSpeed = liftSpeed * .75;
					}

					md2.setM2Speed(liftSpeed);
				}
			}
	 	}
	 	else
	 	{
	 		if(_encLiftPos >= _liftEncGoal  && liftSpeed > 0)
		 	{
		 		md1.setM1Brake(400);
				md1.setM2Brake(400);
		 	}
		 	else if(_encLiftPos <= (_liftEncMin+5) && liftSpeed < 0)
			{
				md1.setM1Brake(400);
				md1.setM2Brake(400);
			}
			else
			{
			 	//Set Lift Speed, Brake if Zero
				if(liftSpeed > -25 && liftSpeed < 25)
				{
					md2.setM2Brake(400);
				}
				else
				{
					if(_encLiftPos > 40 || _encLiftPos < 5)
					{
						liftSpeed = liftSpeed * .25;
					}
					else if(_encLiftPos > 30 || _encLiftPos < 10)
					{
						liftSpeed = liftSpeed * .50;
					}
					else if(_encLiftPos > 20 || _encLiftPos < 15)
					{
						liftSpeed = liftSpeed * .75;
					}

					md2.setM2Speed(liftSpeed);
				}
			}
 		}
 	}
 	else
 	{
 		//Set Lift Speed, Brake if Zero
		if(liftSpeed > -25 && liftSpeed < 25)
		{
			md2.setM2Brake(400);
		}
		else
		{
			md2.setM2Speed(liftSpeed);
		}
 	}
 	
 	
	//Serial.println(liftSpeed);
 }

 void ResetLift()
 {
 	long oldLiftPos = 1;

	md2.setM2Speed(150);

 	while(oldLiftPos != encLift.read())
 	{
		oldLiftPos = encLift.read();
		delay(200);
	}

	md2.setM2Brake(400);

	encLift.write(0);
 }

 void LiftTime(float liftSpeed, long duration)
 {
 	
 }

 void LiftTo(int position, float liftSpeed)
 {
 	
 }

 void MoveIntake(int inPosition)
 {
 	switch(inPosition)
	{
		case 1:
			//intake1.write(150);
          	//intake2.write(150);
			md2.setM1Speed(400);
			break;
		case -1:
			//intake1.write(30);
          	//intake2.write(30);
			md2.setM1Speed(-400);
			break;
		case 0:
			//intake1.write(90);
          	//intake2.write(90);
			md2.setM1Speed(0);
			break;
	}
 }

 void IntakeTime(long duration)
 {
 	
 }

 void Throw()
 {/*
 	if (_isThrowing)
 	{
	 	md2.setM1Speed(-400);
	 	delay(100);
	 	md2.setM1Speed(0);
	 	_isThrowing = false;
 	}
 	else 
 	{
	 	md2.setM1Speed(400);
	 	delay(400);
	 	md2.setM1Speed(0);
	 	_isThrowing = true;
 	}*/
 }







