#ifndef BUTTON_H
#define BUTTON_H

#include "main.h"

typedef enum {
  BUTTON_NONE,
  BUTTON_SHORT_PRESS,
  BUTTON_LONG_PRESS,
} ButtonAction;

void buttonInit();
void buttonProcess();
void buttonTimerProcess();
ButtonAction buttonGetAction(uint8_t index);
void buttonResetActions();

#endif // #ifndef BUTTON_H
