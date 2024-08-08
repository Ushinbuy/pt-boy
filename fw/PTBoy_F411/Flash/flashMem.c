#include "flashMem.h"
#include <stdio.h>
#include <string.h>
#include "main.h"

#define SECTOR_ADDRESS_START	0x08020000
#define SCORE_MEMMORY_LENGTH	0x3FC
#define TASK_SUCESS_LENGTH		0x4
#define NAME_MEMMORY_LENGTH		0x400
#define SECTOR_SIZE 			0x20000

#define TASK_SUCESS_ADDRESS		SECTOR_ADDRESS_START + SCORE_MEMMORY_LENGTH
#define NAME_ADDRESS_START		TASK_SUCESS_ADDRESS + TASK_SUCESS_LENGTH
#define PICTURE_ADDRESS_START	NAME_ADDRESS_START + NAME_MEMMORY_LENGTH

#define PICTURE_MEMMORY_LENGTH	SECTOR_SIZE - (NAME_MEMMORY_LENGTH + SCORE_MEMMORY_LENGTH)

#define PICTURE_SIZE	1024

#define SCORE_ID	0x11
#define PICTURE_ID	0x22
#define NAME_ID		0x33
#define EMPTY_ID	0xFF

typedef struct{
	uint16_t	id;
	int scoresValue;
} ScoresInMem;

typedef struct{
	uint32_t	id;
	uint8_t name[NAME_LENGTH];
} NameInMem;

typedef struct{
	uint32_t id;
	unsigned char picture[PICTURE_SIZE];
} PictureInMem;

void eraseSector(){
	HAL_FLASH_Unlock();

	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SECTORError;

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	EraseInitStruct.Sector = FLASH_SECTOR_5;
	EraseInitStruct.NbSectors = 1;


	  if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
	  {
		  return ;
	  }

	HAL_FLASH_Lock();
}

static void writeToFlash(uint8_t* pData, uint32_t WriteAddr, uint32_t Size){
	HAL_FLASH_Unlock();

	int i;
	uint32_t *pRecord = (uint32_t*) pData;
	uint32_t flash_address = WriteAddr;
	for (i = 0; i < Size; i += 4, pRecord++, flash_address += 4)
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_address, *pRecord);
	HAL_FLASH_Lock();
}

static void readFromFlash(uint8_t *pData, uint32_t ReadAddr, uint32_t Size) {
	uint32_t *ptr = (uint32_t*) pData;
	uint32_t flash_address = ReadAddr;

	for (int i = 0; i < Size; i += 4, ptr++, flash_address += 4)
		*ptr = *(__IO uint32_t *)flash_address;
}

int readLastScoresFromMem(){
	ScoresInMem scores;
	scores.scoresValue = 0;
	uint32_t currentAddress = SECTOR_ADDRESS_START;
	int previousValue;
	do{
		previousValue = scores.scoresValue;
		readFromFlash((uint8_t *)&scores, currentAddress, sizeof(ScoresInMem));
		currentAddress += sizeof(ScoresInMem);
		if(currentAddress >= (SECTOR_ADDRESS_START + SCORE_MEMMORY_LENGTH)){
			// reached the end of the sector
			break;
		}
	} while(scores.id == SCORE_ID);
	return previousValue;
}

void writeLastScoresInMem(int newValue){
	ScoresInMem scoreForWrite, tempScore;
	scoreForWrite.id = SCORE_ID;
	scoreForWrite.scoresValue = newValue;

	uint32_t currentAddress = SECTOR_ADDRESS_START;
	do{
		readFromFlash((uint8_t *)&tempScore, currentAddress, sizeof(ScoresInMem));
		if(currentAddress >= (SECTOR_ADDRESS_START + SCORE_MEMMORY_LENGTH)){
			// reached the end of the sector

			uint8_t picture[PICTURE_SIZE];
			readLastPictureFromMem(picture);
			uint8_t name[NAME_LENGTH];
			readLastNameFromMem(name);
			uint32_t tasksStates;
			readFromFlash((uint8_t *)&tasksStates, TASK_SUCESS_ADDRESS, 1);
			printf("\n\nwriteLastScoresInMem %d\n\n", picture[3]);
			eraseSector();

			writeLastPictureInMem(picture);
			writeNameInMem(name);
			writeToFlash((uint8_t *)&tasksStates, TASK_SUCESS_ADDRESS, 1);
			currentAddress = SECTOR_ADDRESS_START;
		}
		currentAddress += sizeof(ScoresInMem);
	} while(tempScore.id == SCORE_ID);
	writeToFlash((uint8_t *)&scoreForWrite, currentAddress - sizeof(ScoresInMem), sizeof(ScoresInMem));
}

void readLastPictureFromMem(unsigned char *outDate) {
	PictureInMem picture;
	uint32_t currentAddress = PICTURE_ADDRESS_START;
	uint16_t numOfRead = 0;
	do {
		memcpy(outDate, picture.picture, PICTURE_SIZE);
		readFromFlash((uint8_t*) &picture, currentAddress,
				sizeof(PictureInMem));
		currentAddress += sizeof(PictureInMem);
		if (currentAddress
				>= (PICTURE_ADDRESS_START + PICTURE_MEMMORY_LENGTH)) {
			// reached the end of the sector
			break;
		}
		numOfRead++;
	} while (picture.id == PICTURE_ID);

	if (numOfRead == 1) {
		// it mean that no previous picture in Flash
		memset(outDate, 0xFF, PICTURE_SIZE);
	}
}

void writeLastPictureInMem(unsigned char newPicture[]){
	PictureInMem pictureForWrite, tempPicture;
	pictureForWrite.id = PICTURE_ID;
	memcpy(pictureForWrite.picture, newPicture, PICTURE_SIZE);

	uint32_t currentAddress = PICTURE_ADDRESS_START;
	do{
		readFromFlash((uint8_t *)&tempPicture, currentAddress, sizeof(PictureInMem));
		if(currentAddress >= (PICTURE_ADDRESS_START + PICTURE_MEMMORY_LENGTH)){
			// reached the end of the sector
			int scoresValue = readLastScoresFromMem();
			uint8_t name[NAME_LENGTH];
			readLastNameFromMem(name);
			uint32_t tasksStates;
			readFromFlash((uint8_t *)&tasksStates, TASK_SUCESS_ADDRESS, 1);
			eraseSector();

			writeLastScoresInMem(scoresValue);
			writeNameInMem(name);
			writeToFlash((uint8_t *)&tasksStates, TASK_SUCESS_ADDRESS, 1);
			currentAddress = PICTURE_ADDRESS_START;
		}
		currentAddress += sizeof(PictureInMem);
	} while(tempPicture.id == PICTURE_ID);
	writeToFlash((uint8_t *)&pictureForWrite, currentAddress - sizeof(PictureInMem), sizeof(PictureInMem));
}

uint8_t isNoPictureRecordDefaultInMem(){
	PictureInMem picture;
	uint32_t currentAddress = PICTURE_ADDRESS_START;
	uint16_t numOfRead = 0;
	do {
		readFromFlash((uint8_t*) &picture, currentAddress,
				sizeof(PictureInMem));
		currentAddress += sizeof(PictureInMem);
		if (currentAddress
				>= (PICTURE_ADDRESS_START + PICTURE_MEMMORY_LENGTH)) {
			// reached the end of the sector
			break;
		}
		numOfRead++;
	} while (picture.id == PICTURE_ID);

	if (numOfRead == 1) {
		return 1;
	}
	return 0;
}

void readLastNameFromMem(uint8_t *outDate) {
	NameInMem name;
	name.id = NAME_ID;
	uint32_t currentAddress = NAME_ADDRESS_START;
	uint16_t numOfRead = 0;
	do {
		memcpy(outDate, name.name, NAME_LENGTH);
		readFromFlash((uint8_t*) &name, currentAddress,
				sizeof(NameInMem));
		currentAddress += sizeof(NameInMem);
		if (currentAddress
				>= (NAME_ADDRESS_START + NAME_MEMMORY_LENGTH)) {
			// reached the end of the sector
			break;
		}
		numOfRead++;
	} while (name.id == NAME_ID);

	if (numOfRead == 1) {
		// it mean that no previous name in Flash
		uint8_t default_name[NAME_LENGTH] = {0x55, 0x6E, 0x6E, 0x61, 0x6D, 0x65, 0x64, 0x00};	// Unnamed in ASCII
		memcpy(outDate, default_name, NAME_LENGTH);
	}
}

void writeNameInMem(uint8_t newName[]){
	NameInMem nameForWrite, tempName;
	nameForWrite.id = NAME_ID;
	memcpy(nameForWrite.name, newName, NAME_LENGTH);

	uint32_t currentAddress = NAME_ADDRESS_START;
	do{
		readFromFlash((uint8_t *)&tempName, currentAddress, sizeof(NameInMem));
		if(currentAddress >= (NAME_ADDRESS_START + NAME_MEMMORY_LENGTH)){
			// reached the end of the sector

			uint8_t picture[PICTURE_SIZE];
			readLastPictureFromMem(picture);
			int scoresValue = readLastScoresFromMem();
			printf("\n\nwriteLastScoresInMem %d\n\n", picture[3]);
			uint32_t tasksStates;
			readFromFlash((uint8_t *)&tasksStates, TASK_SUCESS_ADDRESS, 1);
			eraseSector();

			writeLastPictureInMem(picture);
			writeLastScoresInMem(scoresValue);
			writeToFlash((uint8_t *)&tasksStates, TASK_SUCESS_ADDRESS, 1);
			currentAddress = NAME_ADDRESS_START;
		}
		currentAddress += sizeof(NameInMem);
	} while(tempName.id == NAME_ID);
	writeToFlash((uint8_t *)&nameForWrite, currentAddress - sizeof(NameInMem), sizeof(NameInMem));
}


void writeTaskState(uint32_t newStates){
	uint32_t tasksStates;
	readFromFlash((uint8_t *)&tasksStates, TASK_SUCESS_ADDRESS, 1);
	if(newStates != tasksStates){
		writeToFlash((uint8_t *)&newStates, TASK_SUCESS_ADDRESS, 1);
	}
}

void readTaskState(uint32_t *tasksStates){
	readFromFlash((uint8_t *)tasksStates, TASK_SUCESS_ADDRESS, 1);
}
