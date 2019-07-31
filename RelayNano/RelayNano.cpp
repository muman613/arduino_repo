#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

void pin_ISR();

#define FLIP_INTERVAL 		1000
#define BOUNCE_THRESHOLD 	80

typedef enum {
	STATE_INIT,
	STATE_WAITING,
	STATE_RUNNING,
} SYSTEM_STATE;

const int 	led2 	= 12;
const int 	relay1 	= 11;
const int 	button1 = 2;

volatile bool 				flip_flop 		= false;
volatile SYSTEM_STATE 		currentState 	= STATE_INIT;
volatile unsigned long 		prevLEDMillis 	= 0;
volatile unsigned long 		openRelayMillis = 0;
volatile int 				buttonState 	= 0;

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

int getTextWidth(const char * text) {
	short int x1, y1;
	unsigned short int w, h;
	display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
	return w;
}
void displayHeader(const char * header) {
//	short int x1, y1;
//	unsigned short int w, h;
	int w;

	display.setTextColor(WHITE);
	display.setTextSize(2);
	w = getTextWidth(header);
//	display.getTextBounds(header, 0, 0, &x1, &y1, &w, &h);
	display.setCursor((128 - w)/2, 0);
	display.print(header);
	display.setTextSize(1);
	display.display();
}

/**
 * Update the OLED panel with current status information.
 */
void updatePanel() {
	char * state = nullptr;
	if (currentState == STATE_RUNNING) {
		state = "Running";
	} else {
		state = "Waiting";
	}
	int w = getTextWidth(state);
	display.fillRect(0, 16, 128, 48, BLACK);
	display.drawRect(0, 16, 128, 48, WHITE);
	display.setCursor((128 - w)/2, 16 + ((48 - 8) / 2));
	display.print(state);

	if (buttonState == 1) {
		display.fillCircle(8, 55, 4, WHITE);
	}
	display.display();
	return;
}

void setup() {
	Serial.begin(115200);
	Serial.println("Initializing OLED panel...");
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
	display.display();
	delay(1000);
	display.clearDisplay();
	displayHeader("Status");

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

	updatePanel();

	if (currentMillis - prevLEDMillis >= FLIP_INTERVAL) {
	    prevLEDMillis = currentMillis;

		if (flip_flop) {
		    digitalWrite(LED_BUILTIN, LOW);
		} else {
		    digitalWrite(LED_BUILTIN, HIGH);
		}

		flip_flop = !flip_flop;
	}

	digitalWrite(led2, buttonState);

	if (currentState == STATE_RUNNING) {
		if (currentMillis <= openRelayMillis) {
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
				openRelayMillis = millis() + 10000;
			}
		}
	}
}
