#include "button.h"
#include "joystick_settings.h"
#include <stdbool.h>

#define BUTTONS_LONG_PRESS_MS                                1000
#define DEBOUNCE_TIME_MS                                     4

typedef struct {
	GPIO_TypeDef *port;
	uint16_t pin;
	GPIO_PinState pressedState;
} McuPin;

typedef enum {
  BUTTON_NOT_PRESSED,
  BUTTON_WAIT_DEBOUNCE,
  BUTTON_PRESSED,
} ButtonState;

static McuPin buttons[BUTTONS_NUM] = {
		{ JOY_RIGHT_PORT, JOY_RIGHT_PIN, GPIO_PIN_SET},
		{ JOY_LEFT_PORT, JOY_LEFT_PIN, GPIO_PIN_SET },
		{ JOY_DOWN_PORT, JOY_DOWN_PIN, GPIO_PIN_SET },
		{ JOY_UP_PORT, JOY_UP_PIN, GPIO_PIN_SET },
		{ JOY_CENTER_PORT, JOY_CENTER_PIN, GPIO_PIN_SET },
		{ BTN_A_GPIO_Port, BTN_A_Pin, GPIO_PIN_RESET},
		{ BTN_B_GPIO_Port, BTN_B_Pin, GPIO_PIN_RESET },
};

static uint16_t debounceCounter[BUTTONS_NUM];
static bool isButtonWaitRelease[BUTTONS_NUM];
static uint16_t buttonPressCounter[BUTTONS_NUM];

static ButtonAction buttonActions[BUTTONS_NUM];
static ButtonState buttonState[BUTTONS_NUM];
volatile static bool wasPressed[BUTTONS_NUM]; // set TRUE just only in HAL_GPIO_EXTI_Callback


static void portsPolling() {
	uint8_t currentGpioState;
	for (uint8_t i = 0; i < BUTTONS_NUM; i++) {
		if(!wasPressed[i]){
			continue;
		}

		currentGpioState = HAL_GPIO_ReadPin(buttons[i].port, buttons[i].pin);

		switch (buttonState[i]) {

		case BUTTON_NOT_PRESSED:
			if (currentGpioState == buttons[i].pressedState){
				buttonState[i] = BUTTON_WAIT_DEBOUNCE;
				debounceCounter[i] = 0;
			}
			break;

		case BUTTON_WAIT_DEBOUNCE:
			if (debounceCounter[i] == DEBOUNCE_TIME_MS) {
				if (currentGpioState == buttons[i].pressedState) {
					buttonState[i] = BUTTON_PRESSED;
				}
				else {
					buttonState[i] = BUTTON_NOT_PRESSED;
					wasPressed[i] = false;
				}
			}
			break;

		case BUTTON_PRESSED:
			if (currentGpioState != buttons[i].pressedState) {
				buttonState[i] = BUTTON_WAIT_DEBOUNCE;
				debounceCounter[i] = 0;
			}
			break;

		default:
			break;
		}
	}
}

static void checkTimers() {
	for (uint8_t i = 0; i < BUTTONS_NUM; i++) {
		if (buttonActions[i] != BUTTON_NONE) {
			continue;
		}
		if (isButtonWaitRelease[i] == false) {
			if (buttonState[i] == BUTTON_PRESSED) {
				isButtonWaitRelease[i] = true;
			}
		}
		else {
			if (buttonPressCounter[i] >= BUTTONS_LONG_PRESS_MS) {
				buttonActions[i] = BUTTON_LONG_PRESS;
				isButtonWaitRelease[i] = false;
			} else if (buttonState[i] == BUTTON_NOT_PRESSED) {
				buttonActions[i] = BUTTON_SHORT_PRESS;
				isButtonWaitRelease[i] = false;
			}
		}
	}
}

void buttonProcess() {
	portsPolling();
	checkTimers();
}

void buttonTimerProcess() {
	for (uint8_t i = 0; i < BUTTONS_NUM; i++) {
		if (debounceCounter[i] < DEBOUNCE_TIME_MS) {
			debounceCounter[i]++;
		}

		if (buttonState[i] == BUTTON_PRESSED) {
			buttonPressCounter[i]++;
		}
		else {
			buttonPressCounter[i] = 0;
		}
	}
}

ButtonAction buttonGetAction(uint8_t index)
{
  return buttonActions[index];
}

void buttonResetActions() {
	for (uint8_t i = 0; i < BUTTONS_NUM; i++) {
		buttonActions[i] = BUTTON_NONE;
	}
}

void buttonInit() {
	for (uint8_t i = 0; i < BUTTONS_NUM; i++) {
		debounceCounter[i] = 0;
		isButtonWaitRelease[i] = false;
		buttonPressCounter[i] = 0;
		buttonActions[i] = BUTTON_NONE;
		buttonState[i] = BUTTON_NOT_PRESSED;
		wasPressed[i] = false;
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	switch(GPIO_Pin){
	case JOY_CENTER_PIN:
		wasPressed[JOY_CENT] = true;
		break;
	case JOY_LEFT_PIN:
		wasPressed[JOY_LEFT] = true;
		break;
	case JOY_RIGHT_PIN:
		wasPressed[JOY_RIGHT] = true;
		break;
	case JOY_UP_PIN:
		wasPressed[JOY_UP] = true;
		break;
	case JOY_DOWN_PIN:
		wasPressed[JOY_DOWN] = true;
		break;
	case BTN_A_Pin:
		wasPressed[BUTTON_A] = true;
		break;
	case BTN_B_Pin:
		wasPressed[BUTTON_B] = true;
		break;
	default:
		break;
	}
}
