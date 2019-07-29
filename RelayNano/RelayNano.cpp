#include <Arduino.h>

void pin_ISR();

#define FLIP_INTERVAL 1000
#define BOUNCE_THRESHOLD 80

typedef enum {
	STATE_INIT,
	STATE_WAITING,
	STATE_RUNNING,
} SYSTEM_STATE;

int led2 = 12;
int relay1 = 11;
int button1 = 2;
bool flip_flop = false;

volatile SYSTEM_STATE currentState = STATE_INIT;

volatile unsigned long previousMillis = 0;

volatile unsigned long stopMillis = 0;

volatile int buttonState = 0;

void setup() {
	Serial.begin(115200);

	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(led2, OUTPUT);
	pinMode(relay1, OUTPUT);
	pinMode(button1, INPUT);
	attachInterrupt(0, pin_ISR, CHANGE);
	currentState = STATE_WAITING;
	digitalWrite(relay1, HIGH);
}

/**
 * Main loop
 */

void loop() {
	unsigned long currentMillis = millis();
	static bool bRunning = false;

	if (currentMillis - previousMillis >= FLIP_INTERVAL) {
	    previousMillis = currentMillis;

		if (flip_flop) {
		    digitalWrite(LED_BUILTIN, LOW);
		} else {
		    digitalWrite(LED_BUILTIN, HIGH);
		}

		flip_flop = !flip_flop;
	}

	digitalWrite(led2, buttonState);

	if (currentState == STATE_RUNNING) {
		if (currentMillis <= stopMillis) {
			if (bRunning != true) {
				Serial.println("running");
			}
			bRunning = true;
			digitalWrite(relay1, LOW);
		} else {
			Serial.println("waiting");
			digitalWrite(relay1, HIGH);
			currentState = STATE_WAITING;
			bRunning = false;
		}
	}
}

/**
 * Interrupt Service Routine for button.
 *
 * Implements a simple debounce algorithm requiring the button up transition after a
 * finite period of ticks.
 *
 */
void pin_ISR() {
	static volatile unsigned long buttonDownMillis = 0;
	static volatile unsigned long buttonUpMillis = 0;

	Serial.println("isr");
	buttonState = digitalRead(button1);
	switch (buttonState) {
	case 0:
		buttonUpMillis = millis();
		break;
	case 1:
		buttonUpMillis = 0;
		buttonDownMillis = millis();
		break;
	}

	if ((buttonUpMillis != 0) && (buttonDownMillis < buttonUpMillis)) {
		if ((buttonUpMillis - buttonDownMillis) > BOUNCE_THRESHOLD) {
			if (currentState == STATE_WAITING) {
				currentState = STATE_RUNNING;
				stopMillis = millis() + 10000;
			}
		}
	}
}
