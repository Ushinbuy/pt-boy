#ifndef CREATEFLAGS_H_
#define CREATEFLAGS_H_

#include <stdint.h>

#define FLAG1_LENGTH 	0x20
#define FLAG2_LENGTH 	0x20
#define KEY_LENGTH 		0x20

// This function generate flag1, flag2 and key
// Flags has format "flag{..........................}" where "." is random values
// Realization of this function is hidden
void generateFlags(uint8_t* flag1, uint8_t* key, uint8_t* flag2);

// Realization of this functions is hidden
void getStatus(uint8_t* Buf, char* outBuf);
void taskSuccess(uint8_t task_num);
uint8_t isTaskSuccess(uint8_t task_num);

#endif /* CREATEFLAGS_H_ */
