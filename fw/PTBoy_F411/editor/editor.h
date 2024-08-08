#ifndef EDITOR_H_
#define EDITOR_H_

#include "main.h"
#include "processState.h"

void editorInit();
ProcessState editorExecute(PushedButton handle);
ProcessState editorMenu(PushedButton handle);

#endif /* EDITOR_H_ */
