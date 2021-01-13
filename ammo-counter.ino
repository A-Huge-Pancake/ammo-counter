//Pancake Customs Ammo Counter Firmware
//Make sure to use Arduino with the board set to 'Arduino Leonardo'.
//Use a micro-usb cable to connect to the port hidden within the Main Module.

/*
 * This script/function is provided AS IS without warranty of any kind. Author(s) disclaim all implied warranties including, without limitation, 
 * any implied warranties of merchantability or of fitness for a particular purpose. The entire risk arising out of the use or performance of the 
 * sample scripts and documentation remains with you. In no event shall author(s) be held liable for any damages whatsoever (including, without 
 * limitation, damages for loss of business profits, business interruption, loss of business information, or other pecuniary loss) arising out of 
 * the use of or inability to use the script or documentation. Neither this script/function, nor any part of it other than those parts that are 
 * explicitly copied from others, may be republished without author(s) express written permission. Author(s) retain the right to alter this disclaimer 
 * at any time.
 * Pancake Customs cannot be held responsible for any damage sustained to the product when it's either been brought onto an airsoft field, or if the
 * case has been opened up. Messing around with the firmware is entirely at your own risk, and is advisable NOT to do so unless you are comfortable
 * with editing C-styled code, and dealing with Arduino-based projects.
 */

//You'll need these libraries installed
//Arduino is default
//U8g2 runs the display
//Wire allows for intrrupts to be used
//EEPROM allows for the microcontroller's EEPROM memory to be used
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <EEPROM.h>

//Initialise the display
U8G2_SSD1306_64X48_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

//Set variables
int ammo = 0;
int preset = 0;
int counterMode = 0;
int rotation = 0;
bool editMode = false;
int editTimer = 0;
bool editShow = false;
int flashTimer = 0;
int w = 0;
bool reloaded = false;
int reloadTimer = 0;
bool reloadShow = false;
int ruptPin = A0;
volatile int ruptVal = 0;
volatile bool ruptBuffer = false;
const int buttonPinLeft = 11;
const int buttonPinMiddle = 10;
const int buttonPinRight = 9;
int buttonStateLeft = 0;
int buttonStateMiddle = 0;
int buttonStateRight = 0;
bool buttonBufferLeft = false;
bool buttonBufferMiddle = false;
bool buttonBufferRight = false;
int pushTimerLeft = 0;
int pushTimerMiddle = 0;
int pushTimerRight = 0;
bool buttonTapRight = false;
bool buttonTapMiddle = false;
bool buttonTapLeft = false;
const int PRESET_ARRAY_SIZE = 10;
const int START_EEPROM_ADDRESS = 17;
const int ROTATE_EEPROM_ADDRESS = 15;
int presetValues[PRESET_ARRAY_SIZE] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

//Write to EEPROM
void writeIntArrayIntoEEPROM(int address, int numbers[], int arraySize){
	int addressIndex = address;
	for(int i = 0; i < arraySize; i++){
		EEPROM.write(addressIndex, numbers[i] >> 8);
		EEPROM.write(addressIndex + 1, numbers[i] & 0xFF);
		addressIndex += 2;
	}
}

//Read from EEPROM
void readIntArrayFromEEPROM(int address, int numbers[], int arraySize){
	int addressIndex = address;
	for (int i = 0; i < arraySize; i++){
		numbers[i] = (EEPROM.read(addressIndex) << 8) + EEPROM.read(addressIndex + 1);
		addressIndex += 2;
	}
}

//Test to see if all the presets are the same (if they are assume the module has not been used yet)
int theArray;
int arraySize;
bool all_are_same(int theArray[], int arraySize){
	for(int i = 1; i < arraySize; i++){
		if(theArray[0] != theArray[i]){
			return false;
		}else{
			return true;
		}
	}
}

void setup(){
	//1.5 second delay
	delay(1500);
	
	//Set up the sensor's interrupt function
	attachInterrupt(digitalPinToInterrupt(1), ammoSpent, CHANGE);
	
	//Set up the display and buttons
	u8g2.begin();
	pinMode(buttonPinLeft, INPUT);
	pinMode(buttonPinMiddle, INPUT);
	pinMode(buttonPinRight, INPUT);
	
	//Perform initial read from EEPROM upon boot
	readIntArrayFromEEPROM(START_EEPROM_ADDRESS, presetValues, PRESET_ARRAY_SIZE);
	
	//Apply values to presets
	ammo = presetValues[preset];
	if(all_are_same(presetValues, PRESET_ARRAY_SIZE)){
		for(int i = 0; i < PRESET_ARRAY_SIZE; i++){
			presetValues[i] = 1;
		}
	}

	//Read what rotation the user last used
	if(EEPROM.read(ROTATE_EEPROM_ADDRESS) == 0xff){
		EEPROM.write(ROTATE_EEPROM_ADDRESS, 0);
	}

	//Set that rotation
	rotation = EEPROM.read(ROTATE_EEPROM_ADDRESS);
}

//If 1 ammo is detected to have passed through the sensor
void ammoSpent(){
	if(!ruptBuffer){
		ammo -= 1;
		ruptBuffer = true;
	}else if(ruptBuffer){
		ruptBuffer = false;
	}
}

//General purpose function to show the main ammo number on every mode
void showAmmo(int method){
	if(method < 1000){
		u8g2.setFont(u8g2_font_fub20_tn);
	}else if(method >= 1000){
		u8g2.setFont(u8g2_font_crox4hb_tf);
	}
	char output[5];
	sprintf(output, "%d", method);
	w = u8g2.getStrWidth(output);
	setRotation(rotation, (64 - w) / 2, 28, 25, (48 - w) / 2, (64 + w) / 2, 20, 39, (48 + w) / 2);
	u8g2.print(method);
}

//General purpose function to show the preset number on every mode
void showPreset(){
	u8g2.setFont(u8g2_font_profont12_tf);
	setRotation(rotation, 1, 8, 55, 1, 63, 39, 8, 47);
	//u8g2.setCursor(1, 8);
	u8g2.print(preset);
}

//General purpse function to rotate display elements depending on a set of values given to it
void setRotation(int rotation, int posX, int posY, int posX90, int posY90, int posX180, int posY180, int posX270, int posY270){
	u8g2.setFontDirection(rotation);
	switch(rotation){
		case 0:
			u8g2.setCursor(posX, posY);
		break;
		case 1:
			u8g2.setCursor(posX90, posY90);
		break;
		case 2:
			u8g2.setCursor(posX180, posY180);
		break;
		case 3:
			u8g2.setCursor(posX270, posY270);
		break;
	}
}

//Begin main loop
void loop(){
	
	//Default (Counting) mode
	if(counterMode == 0){

		//Reset all drawing buffers
		u8g2.clearBuffer();

		//Show the ammo
		if(!reloaded){
			showAmmo(ammo);
		}

		//Show the preset
		showPreset();

		//Display the RELOAD flashing warning if the ammo reaches '0'
		if(ammo == 0){
			reloadTimer++;
			if(reloadTimer % 5 == 0){
				reloadShow = !reloadShow;
			}
			if(reloadShow){
				w = u8g2.getStrWidth("RELOAD");
				setRotation(rotation, (64 - w) / 2, 48, 0, (48 - w) / 2, (64 + w) / 2, 0, 64, (48 + w) / 2);
				u8g2.print("RELOAD");
				switch(rotation){
					case 0:
						u8g2.drawLine(7, 46, 9, 44);
						u8g2.drawLine(9, 44, 11, 46);
						u8g2.drawLine(51, 46, 53, 44);
						u8g2.drawLine(53, 44, 55, 46);
					break;
					case 1:
						u8g2.drawLine(30, 3, 32, 1);
						u8g2.drawLine(32, 1, 34, 3);
					break;
					case 2:
						u8g2.drawLine(8, 3, 10, 1);
						u8g2.drawLine(10, 1, 12, 3);
						u8g2.drawLine(51, 3, 53, 1);
						u8g2.drawLine(53, 1, 55, 3);
					break;
					case 3:
						u8g2.drawLine(30, 3, 32, 1);
						u8g2.drawLine(32, 1, 34, 3);
					break;
				}
			}

		//Display the word RELOAD with arrows when there's still ammo remaining
		}else{
			w = u8g2.getStrWidth("RELOAD");
			setRotation(rotation, (64 - w) / 2, 48, 0, (48 - w) / 2, (64 + w) / 2, 0, 64, (48 + w) / 2);
			u8g2.print("RELOAD");
			switch(rotation){
				case 0:
					u8g2.drawLine(7, 46, 9, 44);
					u8g2.drawLine(9, 44, 11, 46);
					u8g2.drawLine(51, 46, 53, 44);
					u8g2.drawLine(53, 44, 55, 46);
				break;
				case 1:
					u8g2.drawLine(3, 3, 5, 1);
					u8g2.drawLine(5, 1, 7, 3);
				break;
				case 2:
					u8g2.drawLine(8, 3, 10, 1);
					u8g2.drawLine(10, 1, 12, 3);
					u8g2.drawLine(51, 3, 53, 1);
					u8g2.drawLine(53, 1, 55, 3);
				break;
				case 3:
					u8g2.drawLine(57, 3, 59, 1);
					u8g2.drawLine(59, 1, 61, 3);
				break;
			}
		}

		//Display the flashing RELOADED when the user has reloaded
		if(reloaded){
			reloadTimer++;
			if(reloadTimer % 5 == 0){
				reloadShow = !reloadShow;
			}
			if(reloadShow){
				u8g2.setFont(u8g2_font_profont12_tf);
				w = u8g2.getStrWidth("RELOADED");
				setRotation(rotation, (64 - w) / 2, 20, 37, (48 - w) / 2, (64 + w) / 2, 28, 27, (48 + w) / 2);
				u8g2.print("RELOADED");
				if(reloadTimer >= 20){
					reloadTimer = 0;
					reloadShow = false;
					reloaded = false;
				}
			}
		}

		//Send the display code to the display buffer
		u8g2.sendBuffer();

		//Read any button inputs
		buttonStateLeft = digitalRead(buttonPinLeft);
		buttonStateRight = digitalRead(buttonPinRight);
		buttonStateMiddle = digitalRead(buttonPinMiddle);

		//If left button pressed, go to preset selection mode
		if(buttonStateLeft == LOW){
			buttonBufferLeft = false;
		}else if(!buttonBufferLeft){
			counterMode = 1;
			buttonBufferLeft = true;
		}

		//If middle button pressed, reset counter
		if(buttonStateMiddle == LOW){
			buttonBufferMiddle = false;
		}else if(!buttonBufferMiddle){
			reloadTimer = 0;
			reloadShow = false;
			reloaded = true;
			ammo = presetValues[preset];
			buttonBufferMiddle = true;
		}

		//If right button pressed, go to preset selection mode
		if(buttonStateRight == LOW){
			if(buttonTapRight){
				counterMode = 1;
				pushTimerRight = 0;
				buttonTapRight = false;
				buttonBufferRight = false;
			}
			buttonBufferRight = false;

		//If right button held down, rotate display and record it in memory
		}else if(!buttonBufferRight){
			pushTimerRight++;
			if(pushTimerRight < 100){
				buttonTapRight = true;
			}else{
				if(rotation != 3){
					rotation += 1;
					EEPROM.write(ROTATE_EEPROM_ADDRESS, rotation);
				}else{
					rotation = 0;
					EEPROM.write(ROTATE_EEPROM_ADDRESS, rotation);
				}
				buttonTapRight = false;
				pushTimerRight = 0;
				buttonBufferRight = true;
			}
		}

		//Stop the ammo from decrementing any lower than 0
		if(ammo < 0){
			ammo = 0;
		}

	//Preset selection mode
	}else if(counterMode == 1){

		//Reset all drawing buffers
		u8g2.clearBuffer();

		//If were are currently not in preset editing mode
		if(!editMode){

			//Show the ammo and preset number as normal
			showAmmo(presetValues[preset]);
			showPreset();

			//Work out which way around they are supposed to be displayed based on the user's rotation setting
			if(preset != 0){
				switch(rotation){
					case 0:
						u8g2.drawLine(4, 21, 0, 25);
						u8g2.drawLine(0, 25, 4, 29);
					break;
					case 1:
						u8g2.drawLine(21, 4, 25, 0);
						u8g2.drawLine(25, 0, 29, 4);
					break;
					case 2:
						u8g2.drawLine(59, 27, 63, 23);
						u8g2.drawLine(63, 23, 59, 19);
					break;
					case 3:
						u8g2.drawLine(31, 43, 35, 47);
						u8g2.drawLine(35, 47, 39, 43);
					break;
				}
			}
			if(preset != 9){
				switch(rotation){
					case 0:
						u8g2.drawLine(59, 21, 63, 25);
						u8g2.drawLine(63, 25, 59, 29);
					break;
					case 1:
						u8g2.drawLine(21, 43, 25, 47);
						u8g2.drawLine(25, 47, 29, 43);
					break;
					case 2:
						u8g2.drawLine(4, 21, 0, 25);
						u8g2.drawLine(0, 25, 4, 29);
					break;
					case 3:
						u8g2.drawLine(31, 4, 35, 0);
						u8g2.drawLine(35, 0, 39, 4);
					break;
				}
			}

			//Draw the 'select' text
			u8g2.setFont(u8g2_font_profont12_tf);
			w = u8g2.getStrWidth("SELECT");
			setRotation(rotation, (64 - w) / 2, 48, 0, (48 - w) / 2, (64 + w) / 2, 0, 64, (48 + w) / 2);
			u8g2.print("SELECT");

		//Otherwise if in preset editing mode
		}else{

			//Draw the same as before, but flashing
			editTimer++;
			if(editTimer % 5 == 0){
				editShow = !editShow;
			}
			if(editShow){
				showAmmo(ammo);
				showPreset();
			}

			//If the user is at 0, don't show a left marker
			if(ammo != 0){
				u8g2.drawLine(5, 20, 0, 25);
				u8g2.drawLine(0, 25, 5, 30);
			}

			//If the user is at 9999, don't show a right marker
			if(ammo != 9999){
				u8g2.drawLine(58, 20, 63, 25);
				u8g2.drawLine(63, 25, 58, 30);
			}

			//Display the EDITING text
			u8g2.setFont(u8g2_font_profont12_tf);
			w = u8g2.getStrWidth("EDITING");
			setRotation(rotation, (64 - w) / 2, 48, 0, (48 - w) / 2, (64 + w) / 2, 0, 64, (48 + w) / 2);
			u8g2.print("EDITING");
		}

		//Send display code to display buffer
		u8g2.sendBuffer();

		//Read any button inputs
		buttonStateLeft = digitalRead(buttonPinLeft);
		buttonStateRight = digitalRead(buttonPinRight);
		buttonStateMiddle = digitalRead(buttonPinMiddle);

		//If we're not in preset editing mode
		if(!editMode){

			//If the top button is pressed, select the preset and return to default mode
			if(buttonStateMiddle == LOW){
				if(buttonTapMiddle){
					ammo = presetValues[preset];
					pushTimerMiddle = 0;
					buttonTapMiddle = false;
					editMode = false;
					buttonBufferMiddle = false;
					counterMode = 2;
				}
				buttonBufferMiddle = false;
			}else if(!buttonBufferMiddle){
				pushTimerMiddle++;

				//If the top button is held down, go to preset editing mode
				if(pushTimerMiddle < 20){
					buttonTapMiddle = true;
				}else{
					buttonTapMiddle = false;
					editMode = true;
					ammo = presetValues[preset];
					pushTimerMiddle = 0;
					buttonBufferMiddle = true;
				}
			}

			//If the left button is pressed, go to the previous preset
			if(buttonStateLeft == LOW){
				buttonBufferLeft = false;
			}else if(!buttonBufferLeft){
				if(preset != 0){
					preset -= 1;
					buttonBufferLeft = true;
				}
			}

			//If the right button is pressed, go to the next preset
			if(buttonStateRight == LOW){
				buttonBufferRight = false;
			}else if(!buttonBufferRight){
				if(preset != 9){
					preset += 1;
					buttonBufferRight = true;
				}
			}

		//Otherwise if in editing mode
		}else{

			//If the top button is pressed, confirm the user's input value and save it to EEPROM
			if(buttonStateMiddle == LOW){
				buttonBufferMiddle = false;
			}else if(!buttonBufferMiddle){
				presetValues[preset] = ammo;
				writeIntArrayIntoEEPROM(START_EEPROM_ADDRESS, presetValues, PRESET_ARRAY_SIZE);
				buttonBufferMiddle = true;
				pushTimerMiddle = 0;
				counterMode = 3;
			}

			//If the left button is pressed
			if(buttonStateLeft == HIGH && !buttonBufferLeft){
				pushTimerLeft++;
				if(pushTimerLeft < 20){
					buttonTapLeft = true;
				}else{
					buttonTapLeft = false;
				}

				//If it's held down for longer than 20 steps, decrease it by increasingly larger amounts
				if(pushTimerLeft >= 20 && pushTimerLeft < 100){
					ammo -= 1;
				}else if(pushTimerLeft >= 100 && pushTimerLeft < 200){
		        	ammo -= 2;
				}else if(pushTimerLeft > 200){
		        	ammo -= 10;
				}
			}else{
				//Otherwise just decrease by 1
				if(buttonTapLeft){
					ammo -= 1;
					buttonTapLeft = false;
				}
				pushTimerLeft = 0;
				buttonBufferLeft = false;
			}

			//If the right button is pressed
			if(buttonStateRight == HIGH && !buttonBufferRight){
				pushTimerRight++;
				if(pushTimerRight < 20){
					buttonTapRight = true;
				}else{
					buttonTapRight = false;
				}

				//If it's held down for longer than 20 steps, increase it by increasingly larger amounts
				if(pushTimerRight >= 20 && pushTimerRight < 100){
					ammo += 1;
				}else if(pushTimerRight >= 100 && pushTimerRight < 200){
		        	ammo += 2;
				}else if(pushTimerRight > 200){
		        	ammo += 10;
				}
			}else{
				//Otherwise just increase by 1
				if(buttonTapRight){
					ammo += 1;
					buttonTapRight = false;
				}
				pushTimerRight = 0;
				buttonBufferRight = false;
			}

			//Prevent the user from making the ammo count any lower than 0, or any higher than 9999
			if(ammo > 9999){
				ammo = 9999;
			}
			if(ammo < 0){
				ammo = 0;
			}
		}

	//The mode the user breifly enters that shows the PRESET SELECTED screen
	}else if(counterMode == 2){

		//Clear the display buffer
		u8g2.clearBuffer();

		//Set up a timer
		flashTimer++;

		//Draw the text
		u8g2.setFont(u8g2_font_profont12_tf);
		w = u8g2.getStrWidth("PRESET");
		setRotation(rotation, (64 - w) / 2, 20, 37, (48 - w) / 2, (64 + w) / 2, 28, 27, (48 + w) / 2);
		u8g2.print("PRESET");
		w = u8g2.getStrWidth("SELECTED");
		setRotation(rotation, (64 - w) / 2, 40, 11, (48 - w) / 2, (64 + w) / 2, 8, 53, (48 + w) / 2);
		u8g2.print("SELECTED");

		//Once it's showed for 30 steps, go back to default mode
		if(flashTimer > 30){
			flashTimer = 0;
			counterMode = 0;
		}

		//Send the display code to the display buffer
		u8g2.sendBuffer();

	//The mode the user breifly enters taht shows the PRESET SAVED screen
	}else if(counterMode == 3){

		//Clear the display buffer
		u8g2.clearBuffer();

		//Set up a timer
		flashTimer++;

		//Draw the text
		u8g2.setFont(u8g2_font_profont12_tf);
		w = u8g2.getStrWidth("PRESET");
		setRotation(rotation, (64 - w) / 2, 20, 37, (48 - w) / 2, (64 + w) / 2, 28, 27, (48 + w) / 2);
		u8g2.print("PRESET");
		w = u8g2.getStrWidth("SAVED");
		setRotation(rotation, (64 - w) / 2, 40, 11, (48 - w) / 2, (64 + w) / 2, 8, 53, (48 + w) / 2);
		u8g2.print("SAVED");

		//Once it's showed for 30 steps, go back to default mode
		if(flashTimer > 30){
			editMode = false;
		  	flashTimer = 0;
		  	counterMode = 0;
		}

		//Send the display code to the display buffer
		u8g2.sendBuffer();
	}
}
