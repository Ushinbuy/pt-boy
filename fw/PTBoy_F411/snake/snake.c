#include "flashMem.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "snake.h"
#include "ssd1306.h"
#include "pictures.h"

#define NUM_OF_FRUITS 3
#define WALL_THICKNESS 1

static bool gameover;
static uint8_t fruitX[NUM_OF_FRUITS], fruitY[NUM_OF_FRUITS];	// Arrays with the coordinates of the fruits
static uint8_t snakeHeadX, snakeHeadY;
static int score;	// Screen coordinates and variable for points
static uint8_t snakeTailX[256], snakeTailY[256]; // Arrays with the coordinates of the snake's body
static uint8_t nTail;	// Number of pixels that make up the tail of the snake

static uint8_t SNAKE_TIME_UPDATE_MS;//100;
static bool pause;

typedef enum{
	SNAKE_UNKNOWN,
	SNAKE_LEFT,
	SNAKE_RIGHT,
	SNAKE_UP,
	SNAKE_DOWN,
	SNAKE_CENTER
} SnakeDirection;

static SnakeDirection state, prev_state;	// State in which we find ourselves and the one before this

static void gameOver();
static void drawOled();
static void drawPixelBlock(uint8_t x, uint8_t y);
static void moveSnake();
static void spawnFruits();

static void spawnFruits(){	// Spawns a fruit in a random position
	// Check how many fruits are missing and if they are within the limits

	for (uint8_t i = 0; i < NUM_OF_FRUITS; i++)	{
		bool isOutOfScreen = (fruitX[i] <= 2 || fruitY[i] <= 2)
					|| (fruitY[i] >= (SSD1306_HEIGHT - 2) || fruitX[i] >= (SSD1306_WIDTH - 2));

		bool isInHeadOfSnake = (fruitX[i] == snakeHeadX) && (fruitY[i] == snakeHeadY);

		bool isInTailOfSnake = false;
		for (int j = 0; j < nTail; j++) {
			if (snakeTailX[j] == 0xFF || snakeTailY[j] == 0xFF)
				break;
			else if (fruitX[i] == snakeTailX[j] && fruitY[i] == snakeTailY[j]) {
				isInTailOfSnake = true;
				break;
			}
		}

		if (isOutOfScreen || isInHeadOfSnake || isInTailOfSnake) {
			fruitX[i] = (rand() % (SSD1306_WIDTH - 4) + 2) >> 1;
			fruitX[i] <<= 1;
			fruitY[i] = (rand() % (SSD1306_HEIGHT - 4) + 2) >> 1;
			fruitY[i] <<= 1;
			i--;
		}
	}
}

void snakeSetState(PushedButton handle){
	switch(handle){
	case JOY_LEFT:
		if(prev_state != SNAKE_RIGHT && !pause)
			state = SNAKE_LEFT;
		break;
	case JOY_RIGHT:
		if(prev_state != SNAKE_LEFT && !pause)
			state = SNAKE_RIGHT;
		break;
	case JOY_DOWN:
		if(prev_state != SNAKE_UP && !pause)
			state = SNAKE_DOWN;
		break;
	case JOY_UP:
		if(prev_state != SNAKE_DOWN && !pause)
			state = SNAKE_UP;
		break;
	case JOY_CENT:
		state = SNAKE_CENTER;
		break;
	default:
		break;
	}
}

void snakeSetup()	//  Game variables initialization function
{
	ssd1306_Fill(Black);
	ssd1306_SetCursor(24, 4);
	ssd1306_WriteString("SNAKE", Font_16x26, White);
	ssd1306_SetCursor(24, 34);
	ssd1306_WriteString("START", Font_16x26, White);
	ssd1306_UpdateScreen();
	HAL_Delay(1000);

	char string[100];

	// Variables
	gameover = false;
	pause = false;

	state = SNAKE_LEFT;
	prev_state = SNAKE_UNKNOWN;
	snakeHeadX = SSD1306_WIDTH / 2;
	snakeHeadY = SSD1306_HEIGHT / 2;

	nTail = 4;

	memset(snakeTailX, 0xFF, 256);
	memset(snakeTailY, 0xFF, 256);
	for(uint8_t i = 0; i < nTail; i++){
		snakeTailX[i] = SSD1306_WIDTH / 2 + i * 2;
		snakeTailY[i] = SSD1306_HEIGHT / 2;
	}

	// Reset the score
	score = 0;
	sprintf(string, "%d", score);

	// Reset the coordinates of the fruits
	for(uint8_t i = 0; i < NUM_OF_FRUITS; i++){
		fruitX[i] = 0;
		fruitY[i] = 0;
	}
	spawnFruits();	// We generate 3 new fruits

	SNAKE_TIME_UPDATE_MS = 200;

	drawOled();
}

static void moveSnake(){
	// Left - Variation in the X axis
	if (state == SNAKE_LEFT) {
		snakeHeadX -= 2;
		prev_state = SNAKE_LEFT;
	}	//Right
	else if (state == SNAKE_RIGHT) {
		snakeHeadX += 2;
		prev_state = SNAKE_RIGHT;
	}	// Up - Variation in the Y axis
	else if (state == SNAKE_UP) {
		snakeHeadY -= 2;
		prev_state = SNAKE_UP;
	}	// Down
	else if (state == SNAKE_DOWN) {
		snakeHeadY += 2;
		prev_state = SNAKE_DOWN;
	}
}

ProcessState snakeExecute(){	// Main function that defines the operation
	static uint32_t oldTickValue = 0;
	if ((HAL_GetTick() - oldTickValue) < SNAKE_TIME_UPDATE_MS) {
		return PROCESS_EXECUTE;
	}

	oldTickValue = HAL_GetTick();


	if (gameover) {
		gameOver();
		return PROCESS_FINISHED;
	}

	uint8_t prevX, prevY, prev2X, prev2Y;

	// Update the queue
	if ((state != SNAKE_CENTER) && !pause)
	{
		prevX = snakeTailX[0];
		prevY = snakeTailY[0];
		snakeTailX[0] = snakeHeadX;
		snakeTailY[0] = snakeHeadY;

		for (uint8_t i = 1; i < nTail; i++)
		{
			prev2X = snakeTailX[i];
			prev2Y = snakeTailY[i];
			snakeTailX[i] = prevX;
			snakeTailY[i] = prevY;
			prevX = prev2X;
			prevY = prev2Y;
		}
	}

	moveSnake();

	// Game paused when middle button is pressed
	if (state == SNAKE_CENTER && !gameover)
	{
		if(pause){
			pause = false;
			state = prev_state;
		}
		else{
			pause = true;
			state = SNAKE_UNKNOWN;
		}
	}

	// Game Over Situations
	if(!pause){
		if (snakeHeadX >= (SSD1306_WIDTH-3)) // Running into the side walls
			gameover = true;
		else if (snakeHeadX <= 1)
			gameover = true;

		if (snakeHeadY >= (SSD1306_HEIGHT-3)) // Hitting the top or bottom wall
			gameover = true;
		else if (snakeHeadY <= 1)
			gameover = true;

		for (uint8_t i = 0; i < nTail; i++) // Collides with its own tail
		{
			if (snakeTailX[i] == snakeHeadX && snakeTailY[i] == snakeHeadY)
				gameover = true;
		}
	}

	const uint8_t SNAKE_TIME_DECREMENT = 30;
	// Check if a fruits has been eaten
	for (uint8_t i = 0; i < NUM_OF_FRUITS; i++)
	{
		if (snakeHeadX == fruitX[i] && snakeHeadY == fruitY[i])
		{
			score++;
			spawnFruits();
			nTail++;
			if (score % 10 == 0 && SNAKE_TIME_UPDATE_MS > 30){
				SNAKE_TIME_UPDATE_MS -= SNAKE_TIME_DECREMENT;
				oldTickValue = HAL_GetTick();
			}
		}
	}

	drawOled();
	return PROCESS_EXECUTE;
}

void drawOled(){
	ssd1306_Fill(Black);	// Clean the screen

	// Draw the walls
	// x lines
	ssd1306_DrawLine(1, 1, SSD1306_WIDTH-2, 1, White);
	ssd1306_DrawLine(1, SSD1306_HEIGHT-2, SSD1306_WIDTH-2, SSD1306_HEIGHT-2, White);

	// y lines
	ssd1306_DrawLine(1, 1, 1, SSD1306_HEIGHT-2, White);
	ssd1306_DrawLine(SSD1306_WIDTH-2, 1, SSD1306_WIDTH-2, SSD1306_HEIGHT-2, White);

	// Draw the fruits and the snake
	for (uint8_t x_pix = 2; x_pix < SSD1306_WIDTH - 2; x_pix+=2)
	{
		for (uint8_t y_pix = 2; y_pix < SSD1306_HEIGHT - 2; y_pix+=2)
		{
			// Draw Snake
			if (x_pix == snakeHeadX && y_pix == snakeHeadY){
				// main pixel
				drawPixelBlock(x_pix,y_pix);
			}
			else
			{
				// tail
				for (uint8_t k = 0; k < nTail; k++)
				{
					if (snakeTailX[k] == x_pix && snakeTailY[k] == y_pix)
					{
						drawPixelBlock(x_pix,y_pix);
					}
				}
			}

			// Draw fruits
			for(uint8_t num = 0; num < NUM_OF_FRUITS; num++){
				if (x_pix == fruitX[num] && y_pix == fruitY[num]){
					drawPixelBlock(x_pix,y_pix);
				}
				if(num == NUM_OF_FRUITS)
					printf("some problems");
			}
		}
	}

	if(state == SNAKE_UNKNOWN && pause){
		char string[16];
		ssd1306_SetCursor(36, 12);
		ssd1306_WriteString("PAUSE", Font_11x18, White);

		sprintf(string, "Score %d", score);
		ssd1306_SetCursor(18, 28);
		ssd1306_WriteString(string, Font_11x18, White);
	}

	ssd1306_UpdateScreen();
}

void gameOver(){		// GameOver screen
	char string[100];

	// We draw the main screen of Game Over

	ssd1306_Fill(Black);
	ssd1306_SetCursor(14, 1);
	ssd1306_WriteString("GAME OVER", Font_11x18, White);
	ssd1306_SetCursor(14, 20);
	sprintf(string, "Score: %2d", score);
	ssd1306_WriteString(string, Font_11x18, White);

	uint8_t oldScoresRecord = readLastScoresFromMem();
	if (score > oldScoresRecord) {
		ssd1306_SetCursor(26, 42);
		ssd1306_WriteString("NEW RECORD!", Font_7x10, White);
		if(picturesIsNewPicureUnlock(oldScoresRecord, score) == 1 && oldScoresRecord != 0){
			ssd1306_SetCursor(8, 54);
			ssd1306_WriteString("PICTURE UNLOCKED", Font_7x10, White);
		}
		writeLastScoresInMem(score);
	}
	ssd1306_UpdateScreen();
}

void drawPixelBlock(uint8_t x, uint8_t y){
	ssd1306_DrawPixel(x, y, White);
	ssd1306_DrawPixel(x + 1, y, White);
	ssd1306_DrawPixel(x, y + 1, White);
	ssd1306_DrawPixel(x + 1, y + 1, White);
}
