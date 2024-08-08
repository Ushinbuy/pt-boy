#ifndef EEPROMSPACE_H_
#define EEPROMSPACE_H_

#include <stdint.h>

#define NAME_LENGTH 8

void eraseSector();

int readLastScoresFromMem();
void writeLastScoresInMem(int newValue);

void readLastPictureFromMem(unsigned char *outDate);
void writeLastPictureInMem(unsigned char newPicture[]);
uint8_t isNoPictureRecordDefaultInMem();

void readLastNameFromMem(uint8_t *outDate);
void writeNameInMem(uint8_t newName[]);

void writeTaskState(uint32_t tasksStates);
void readTaskState(uint32_t *tasksStates);

#endif /* EEPROMSPACE_H_ */
