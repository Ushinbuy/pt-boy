#include "flashMem.h"
#include "morePictures.h"
#include "pictures.h"
#include "ssd1306.h"
#include <string.h>
#include <stdio.h>

#include "pt_logo.h"
#include "offzone_logo.h"
#include "shit.h"
#include "addonPictures.h"
#include "pics2024.h"
#include "createflags.h"
#include "new.h"

#define NUM_OF_PICTURES 17 + 3 // 3 new  added in 2024

typedef struct {
	uint8_t picture[1024];
	uint8_t minScore;
} PicWithLock;

typedef enum{
	SCROLLING_DISABLE,
	SCROLLING_ENABLE,
} ScrollingState;

typedef enum{
	PIC_NOT_LOCKED,
	PIC_LOCK_BY_SCORES,
	PIC_LOCK_BY_TASKS
} PicLockedBy;

static PicWithLock picSet[NUM_OF_PICTURES];
static ScrollingState scrollingState = SCROLLING_ENABLE;
static uint32_t oldTickValue = 0;
static int8_t currentPictureIndex = 0;

static PicLockedBy isPictureLocked();
static void picturesViewLock(int needScores);
static void picturesViewLockByTask(void);
static void redrawPicture();

void picturesShowLogoAnimation(){
	ssd1306_Fill(Black);
	for(int i = 0; i < pt_logo_allArray_LEN ; i++){
		ssd1306_SetPicture(pt_logo_allArray[i]);
		HAL_Delay(75);
	}

	HAL_Delay(1200);
}

void picturesSetupSlideShow(){
	for (uint8_t i = 0; i < NUM_OF_PICTURES; i++){
		picSet[i].minScore = 0;
	}

	int inc = 0;
	memcpy(picSet[inc].picture, pl, 1024);

	inc++;
	memcpy(picSet[inc].picture, OFFZONE_logo, 1024);

	inc++;
	memcpy(picSet[inc].picture, addon_image_03, 1024);

	inc++;
	memcpy(picSet[inc].picture, addon_image_02, 1024);

	inc++;
	memcpy(picSet[inc].picture, addon_image_06, 1024);
	picSet[inc].minScore = 50;

	inc++;
	memcpy(picSet[inc].picture, gImage_PWNED, 1024);
	picSet[inc].minScore = 40;

	inc++;
	memcpy(picSet[inc].picture, addon_image_05, 1024);

	inc++;
	memcpy(picSet[inc].picture, addon_image_04, 1024);
	picSet[inc].minScore = 20;

	inc++;
	memcpy(picSet[inc].picture, addon_image_07, 1024);

	inc++;
	memcpy(picSet[inc].picture, addon_image_08, 1024);

	inc++;
	memcpy(picSet[inc].picture, dino_v1, 1024);
	picSet[inc].minScore = 30;

	// EDITOR PICTURE
	inc++;
	if(isNoPictureRecordDefaultInMem()){
		ssd1306_Fill(Black);
		ssd1306_SetCursor(20, 12);
		ssd1306_WriteString("EDIT ME!", Font_11x18, White);
		ssd1306_SetCursor(16, 40);
		ssd1306_WriteString("Press B 1 sec", Font_7x10, White);

		memcpy(picSet[inc].picture, ssd1306_GetBufferAddress(), SSD1306_BUFFER_SIZE);
	}
	else{
		readLastPictureFromMem(picSet[inc].picture);
	}

	inc++;
	memcpy(picSet[inc].picture, addon_image_13, 1024);

	inc++;
	memcpy(picSet[inc].picture, addon_image_14, 1024);

	inc++;
	memcpy(picSet[inc].picture, addon_image_15, 1024);
	picSet[inc].minScore = 10;

	inc++;
	memcpy(picSet[inc].picture, addon_image_16, 1024);

	inc++;
	ssd1306_Fill(Black);
	ssd1306_SetCursor(38, 0);
	ssd1306_WriteString("SNAKE", Font_11x18, White);
	ssd1306_SetCursor(34, 21);
	ssd1306_WriteString("MAX SCORE", Font_7x10, White);

	char str[10];
	sprintf(str, "%03d", readLastScoresFromMem());
	ssd1306_SetCursor(41, 34);
	ssd1306_WriteString(str, Font_16x26, White);

	memcpy(picSet[inc].picture, ssd1306_GetBufferAddress(), SSD1306_BUFFER_SIZE);

	inc++;
	memcpy(picSet[inc].picture, glasses_v1, 1024);

	inc++;
	memcpy(picSet[inc].picture, harold_v3, 1024);

	inc++;
	memcpy(picSet[inc].picture, bug_black, 1024);

	oldTickValue = HAL_GetTick();
	redrawPicture();
}

static void incrementIndex(){
	if(scrollingState == SCROLLING_DISABLE){
		return;
	}
	currentPictureIndex++;
	if(currentPictureIndex > NUM_OF_PICTURES - 1){
		currentPictureIndex = 0;
	}
}

static void decrementIndex(){
	if(scrollingState == SCROLLING_DISABLE){
		return;
	}
	currentPictureIndex--;
	if(currentPictureIndex < 0){
		currentPictureIndex = NUM_OF_PICTURES - 1;
	}
}

static PicLockedBy isPictureLocked(){
	int scores = readLastScoresFromMem();
	if (scores < picSet[currentPictureIndex].minScore)
		return PIC_LOCK_BY_SCORES;
	else if ((!isTaskSuccess(1) && (currentPictureIndex == 17))
			|| (!isTaskSuccess(2) && (currentPictureIndex == 18))
			|| (!isTaskSuccess(3) && (currentPictureIndex == 19))) {
		return PIC_LOCK_BY_TASKS;
	}
	return PIC_NOT_LOCKED;
}

static void redrawPicture(){
	ssd1306_Fill(Black);
	ssd1306_SetPicture(picSet[currentPictureIndex].picture);
	PicLockedBy picLockedBy = isPictureLocked();
	if(picLockedBy == PIC_LOCK_BY_SCORES) {
		picturesViewLock(picSet[currentPictureIndex].minScore);
	}
	else if (picLockedBy == PIC_LOCK_BY_TASKS) {
		picturesViewLockByTask();
	}
}

static void toogleScrolling(){
	scrollingState ^= 1UL;
}

static void picturesShowSlideShow(){
	incrementIndex();
	redrawPicture();
}

void picturesViewLock(int needScores){
	uint8_t lockX = 53;
	uint8_t lockY = 7;
	uint8_t lockWidth = 22;
	uint8_t lockHeight = 32;

	ssd1306_DrawFilledRectangle(26, lockY + lockHeight - 4, 74, 24, Black);

	ssd1306_SetPictureHeightAndWith(lockX, lockY, lock32x20, lockWidth, lockHeight, White);

	ssd1306_SetCursor(28, lockY += lockHeight);
	char string[16];
	sprintf(string, "%03d POINTS",needScores);
	ssd1306_WriteString(string, Font_7x10, White);
	ssd1306_SetCursor(32, lockY + 10);
	ssd1306_WriteString("TO UNLOCK", Font_7x10, White);
	ssd1306_UpdateScreen();
}

void picturesViewLockByTask(void){
	uint8_t lockX = 53;
	uint8_t lockY = 16;
	uint8_t lockWidth = 22;
	uint8_t lockHeight = 32;

	ssd1306_SetPictureHeightAndWith(lockX, lockY, lock32x20, lockWidth, lockHeight, White);

	ssd1306_UpdateScreen();
}

int picturesIsNewPicureUnlock(int oldScores, int newScores){
	for(int i = 0; i < NUM_OF_PICTURES; i++){
		int tableScore = picSet[i].minScore;
		if((tableScore >= oldScores) && (tableScore < newScores)){
			return 1;
		}
	}
	return 0;
}

void picturesExecute(PushedButton handle){
	const uint32_t SLIDESHOW_TIME_UPDATE_MS = 3000;
	if ((HAL_GetTick() - oldTickValue) > SLIDESHOW_TIME_UPDATE_MS) {
		oldTickValue = HAL_GetTick();
		picturesShowSlideShow();
	}
	HAL_Delay(50);
	switch (handle) {
	case JOY_LEFT:
		decrementIndex();
		redrawPicture();
		oldTickValue = HAL_GetTick();
		break;
	case JOY_RIGHT:
		incrementIndex();
		redrawPicture();
		oldTickValue = HAL_GetTick();
		break;
	case JOY_CENT:;
		if(isPictureLocked() == PIC_NOT_LOCKED) {
				toogleScrolling();
		}
		break;
	default:
		break;
	}
}
