#define MorseInPin 3
// HIGH : Not pressed  LOW: MorseKey pressed

#define MorseOutPin 11
// Output to control the light in the tower 

// Maybe we will need pins for light and sound
// #define MorseSoundPin
 #define MorseLightPin 13

#define MorseTonePin 5

#define RecordButtonPin 2
// HIGH: Record not pressed.  LOW: Record button pressed

#define PlayPauseButtonPin 7 
// Toggle. HIGH: Play recorded message.  LOW: Pause playing

// States 
// 0: Just Turned On
// 100: Recording
// 200: Playing
// 300: Paused
int state = 0;
int ArrayPointer = 0;
int ArrayMax = 0;
int ArrayAbsoluteMax = 699;
int MorseArray[700];

unsigned long NextTimeToAdvanceArrayPointer = 0;

// The setup() function runs once each time the micro-controller starts
void setup()
{
	pinMode(MorseInPin, INPUT_PULLUP);
	pinMode(MorseOutPin, OUTPUT);
	pinMode(MorseLightPin, OUTPUT);
	pinMode(RecordButtonPin, INPUT_PULLUP);
	pinMode(PlayPauseButtonPin, INPUT_PULLUP);

	Serial.begin(115200);

	// delay to somehow fix strange difference between build and power back
	delay(500);
}

// Add the main program code into the continuous loop() function
void loop()
{
//	Serial.print("State: "); Serial.println(state);
	static int MorseInPinLastValue = 0;
	static unsigned long LastTimeStateofMorsePinChanged = 0;
	// 0: Just Turned On
	if (state == 0) {
		// We have just had a reset - Fill with a default message so we have something to send
		FillStandardMessage();
		// Prepare for playing
		ArrayPointer = 0;
		NextTimeToAdvanceArrayPointer = 0;
		// Depending on the play/Pause-button, we either go to Play-state or pause-state
		int PlayPauseButtonPinValue = digitalRead(PlayPauseButtonPin);
		if (PlayPauseButtonPinValue == LOW) {
			state = 300; // Pause
		}
		else {
			state = 200;  // Play
		}
	}

	// 200: Play
	if (state == 200) { 

		int PlayPauseButtonPinValue = digitalRead(PlayPauseButtonPin);
		int RecordButtonPinValue = digitalRead(RecordButtonPin);

		// Play until PlayPauseButtonPin toggles low
		if (PlayPauseButtonPinValue == LOW) {
			state = 300;
		}
		else {
			// We never want to say even a single beep, if the pause-button is LOW (Pause-state)
			PlayRecordedMessage();
		}

		// .. or until RecondButton is pressed
		// RecordButton wins
		if (RecordButtonPinValue == LOW) {
			// Stop the tone if it is on
			NoBeep();

			// Record the state and time of when the record button got pressed
			// We want to handle the case, when the record button is pressed while the morsebutton also is pressed
			MorseInPinLastValue = digitalRead(MorseInPin);
			LastTimeStateofMorsePinChanged = millis();
			ArrayPointer = 0;
			state = 100;  // Record
		}

	}
	// 100: Record
	if (state == 100) {
		// Recording
		int RecordButtonPinValue = digitalRead(RecordButtonPin);
		// Read the state of the morseinpin
		int MorseInPinValue = digitalRead(MorseInPin);

		// Compare to last loop - Did we switch state? 

		// If state switched from LOW to HIGH or HIGH to LOW (Button got pressed or released since last time) : 
		unsigned long debounce = millis() - LastTimeStateofMorsePinChanged;
		if (MorseInPinValue != MorseInPinLastValue && debounce > 30  || RecordButtonPinValue == HIGH) {

			// Tone or notone
			if (MorseInPinValue == LOW) {
				Beep();
			}
			else {
				NoBeep();
			}

			int plusminus;
			// Plus or minus of the last event (if we have beep or no beep)
			// Minus is beep is Pin LOW - plus is no beep pin HIGH
			// If we are on the last run (Recording is stopped) we will use the current MorseInPinValue and not the Last
			if (RecordButtonPinValue == HIGH)
			{
				if (MorseInPinValue == LOW) {
					plusminus = -1;
				}
				else {
					plusminus = 1;
				}

			}
			// All other runs, use the last value
			else {
				if (MorseInPinLastValue == LOW) {
					plusminus = -1;
				}
				else {
					plusminus = 1;
				}

			}

			// Calculate elapsed time that passed since last State change 
			unsigned long elapsedTime = millis() - LastTimeStateofMorsePinChanged;
			// Record the time of HIGH (Button Pressed)
			LastTimeStateofMorsePinChanged = millis();
			// divide the elapsed time with 32767 - To handle longer elapsed times than 32 seconds. 
			unsigned long overflows = elapsedTime / 32767;

			// if result > 0 add n to the array with 32767
			if (overflows > 0) {
				int ArrayPointerAtOverflow = ArrayPointer;
				// Make sure we dont overflow the array
				if (overflows + ArrayPointerAtOverflow < ArrayAbsoluteMax) {
					while (ArrayPointer < overflows + ArrayPointerAtOverflow)
					{
						MorseArray[ArrayPointer] = plusminus * 32767;
						ArrayPointer++;
					}
				}
			}
			// modulus the elapsed time with 32767 and add one to the array with that
			unsigned long rest = elapsedTime % 32767;
			MorseArray[ArrayPointer] = plusminus*rest;

			// Never allow the array to overflow
			if (ArrayPointer < ArrayAbsoluteMax) {
				ArrayPointer++;
			}
			MorseInPinLastValue = MorseInPinValue;
		}

		if (RecordButtonPinValue == HIGH) {
			// The record button got released
			// Finalize
			ArrayMax = ArrayPointer-1;
			// Prepare for playing from the top
			ArrayPointer = 0;
			NextTimeToAdvanceArrayPointer = 0;

			// Depending on the play/Pause-button, we either go to Play-state or pause-state
			int PlayPauseButtonPinValue = digitalRead(PlayPauseButtonPin);
			if (PlayPauseButtonPinValue == LOW) {
				state = 300; // Pause
			}
			else {
				state = 200;  // Play
			}
		}

	}
	// 300: Pause
	if (state == 300)
	{
		// Stop the tone if playing We don't want it hanging


		int PlayPauseButtonPinValue = digitalRead(PlayPauseButtonPin);
		int RecordButtonPinValue = digitalRead(RecordButtonPin);
		// We want to pass the morseInButton presses through in the pause state to make it possible to key manually
		int MorseInPinPauseValue = digitalRead(MorseInPin);

		// No morsekey press, no beep, no nothing
		if (MorseInPinPauseValue == HIGH)
		{
			NoBeep();
		}
		if (MorseInPinPauseValue == LOW)
		{
			Beep();
		}
		// Pause until PausePlay goes HIGH
		if (PlayPauseButtonPinValue == HIGH) {
			// We start from the top.
			ArrayPointer = 0;
			NextTimeToAdvanceArrayPointer = 0;

			state = 200; //Play
		}
		// Pause until Record is pressed (LOW)
		if (RecordButtonPinValue == LOW) {
			MorseInPinLastValue = digitalRead(MorseInPin);
			LastTimeStateofMorsePinChanged = millis();
			ArrayPointer = 0;

			state = 100; // Record
		}

	}

}

  int dih = 150;
  int dah = 450;


void FillStandardMessage() {
	ArrayPointer = 0;

 C_();Q_();__();
 C_();Q_();__();
 C_();Q_();__();
 __();__();__();

 P_();U_();L_();S_();A_();R_();
 __();
 P_();U_();L_();S_();A_();R_();
 __();
 P_();U_();L_();S_();A_();R_();
 __();__();__();

 O_();N_(); __();
 O_();F_();F_(); __();
 __();
 O_();N_(); __();
 O_();F_();F_(); __();
 __();
 O_();N_(); __();
 O_();F_();F_(); __();
 __();__();__();

H_();O_();P_();E_();
__();__();__();

F_();A_();I_();T_();H_();
__();__();__();

P_();E_();A_();C_();E_();
__();__();__();

L_();O_();V_();E_();
__();__();__();

S_();A_();M_();M_();E_();N_(); __(); E_();R_(); __(); V_();I_(); __(); S_();T_();AE_();R_();K_();E_();R_();E_();
__();__();__();

T_();R_();O_(); __(); P_();AA_(); __(); D_();E_();T_(); __(); O_();G_(); __(); G_();I_();V_(); __(); D_();E_();T_(); __(); V_();I_();D_();E_();R_();E_();
__();__();__();

	ArrayMax = ArrayPointer;
	ArrayPointer = 0;

}

void __(){
  MorseArray[ArrayPointer] = dah; ArrayPointer++;
  
}
void A_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;  

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void B_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void C_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void D_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void E_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void F_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void G_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void H_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void I_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void J_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void K_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void L_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void M_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void N_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void O_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void P_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void Q_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void R_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void S_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void T_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void U_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void V_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void W_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void X_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void Y_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;


  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}
void Z_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}

void AE_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}

void OE_(){
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}

void AA_(){
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dih; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;
  MorseArray[ArrayPointer] = -dah; ArrayPointer++;
  MorseArray[ArrayPointer] = dih; ArrayPointer++;

  MorseArray[ArrayPointer] = dih; ArrayPointer++;  
}

void PlayRecordedMessage() {

	// If it is time...
	if (millis() > NextTimeToAdvanceArrayPointer) {
//		Serial.print(" AP "); Serial.println(ArrayPointer);

		// Take value in array of ArrayPointer
		// If value is < 0 set MorseOutPin HIGH (Beep)
		if (MorseArray[ArrayPointer] < 0) {
			NextTimeToAdvanceArrayPointer = millis() - MorseArray[ArrayPointer];
			Beep();
		}
		else {
			NextTimeToAdvanceArrayPointer = millis() + MorseArray[ArrayPointer];
			NoBeep();
		}
		ArrayPointer++;

		// Have we reached the end of the recording?
		if (ArrayPointer > ArrayMax) {
			// Then set the ArrayPointer back to zero
			ArrayPointer = 0;
		}

	}

}

void Beep() {
	// MorseOut to LOW (Ground starts light)
	digitalWrite(MorseOutPin, LOW);

	// Light pin to HIGH to make the light shine
	digitalWrite(MorseLightPin, HIGH);
	// Start Tone
	tone(MorseTonePin, 500);
}

void NoBeep() {
	// MorseOut to HIGH - High = no light
	digitalWrite(MorseOutPin, HIGH);

	/// Light pin to LOW to make the light off
	digitalWrite(MorseLightPin, LOW);
	// NoTone
	noTone(MorseTonePin);

}
