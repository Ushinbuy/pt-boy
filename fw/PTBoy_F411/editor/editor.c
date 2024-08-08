#include "flashMem.h"
#include "editor.h"
#include "ssd1306.h"
#include <stdbool.h>
#include <string.h>
#include "button.h"

static int xCursor, yCursor;
#define PIXEL_SIZE 2 // must be equal to 2

typedef enum{
	EDITOR_BUFFER_CLEAR,
	EDITOR_BUFFER_BACKUP,
	EDITOR_BUFFER_SAVE,
	EDITOR_BUFFER_LOAD,
	EDITOR_BUFFER_RESUME,
} EditorBuffer_t;

static void toogleCursorView();
static void bufferHandle(EditorBuffer_t handler);
static void viewMenu();
static void invertDrawInBuffer();

static bool isCursorView = false;

#define TIME_CURSOR_TOGGLING_MS	300

void editorInit(){
	ssd1306_Fill(Black);
	if(isNoPictureRecordDefaultInMem()){
		ssd1306_Fill(Black);
		ssd1306_SetCursor(20, 12);
		ssd1306_WriteString("EDIT ME!", Font_11x18, White);
		ssd1306_SetCursor(16, 40);
		ssd1306_WriteString("Press B 1 sec", Font_7x10, White);

		bufferHandle(EDITOR_BUFFER_BACKUP);
	}
	else{
		bufferHandle(EDITOR_BUFFER_LOAD);
	}

	ssd1306_UpdateScreen();

	xCursor = SSD1306_WIDTH/2;
	yCursor = SSD1306_HEIGHT/2;
}

static void toogleCursorView(){
	const int offset = 3;
	const int lineLength = 1;

	// - PIXEL_SIZE/2
	// X lines
	ssd1306_DrawIvertedLine(xCursor + offset, yCursor + PIXEL_SIZE/2, xCursor + (offset + lineLength), yCursor + PIXEL_SIZE/2);
	ssd1306_DrawIvertedLine(xCursor + offset, yCursor, xCursor + (offset + lineLength), yCursor);
	ssd1306_DrawIvertedLine(xCursor - offset + PIXEL_SIZE/2, yCursor + PIXEL_SIZE/2, xCursor - (offset + lineLength) + PIXEL_SIZE/2, yCursor + PIXEL_SIZE/2);
	ssd1306_DrawIvertedLine(xCursor - offset + PIXEL_SIZE/2, yCursor, xCursor - (offset + lineLength) + PIXEL_SIZE/2, yCursor);
	// Y lines
	ssd1306_DrawIvertedLine(xCursor + PIXEL_SIZE/2, yCursor + offset, xCursor + PIXEL_SIZE/2, yCursor + (offset + lineLength));
	ssd1306_DrawIvertedLine(xCursor, yCursor + offset, xCursor, yCursor + (offset + lineLength));
	ssd1306_DrawIvertedLine(xCursor + PIXEL_SIZE/2, yCursor - offset + PIXEL_SIZE/2, xCursor + PIXEL_SIZE/2, yCursor - (offset + lineLength) + PIXEL_SIZE/2);
	ssd1306_DrawIvertedLine(xCursor, yCursor - offset + PIXEL_SIZE/2, xCursor, yCursor - (offset + lineLength) + PIXEL_SIZE/2);

	isCursorView ^= true;
	ssd1306_UpdateScreen();
}

ProcessState editorExecute(PushedButton handle){
	// delay need to blocking many buttons signals
	switch (handle) {
		case JOY_LEFT:
			if(isCursorView){
				toogleCursorView();
			}
			xCursor -= PIXEL_SIZE;
			if(xCursor < 0){
				xCursor = SSD1306_WIDTH - PIXEL_SIZE;
			}
			break;
		case JOY_RIGHT:
			if(isCursorView){
				toogleCursorView();
			}
			xCursor += PIXEL_SIZE;
			if(xCursor >= SSD1306_WIDTH){
				xCursor = 0;
			}
			break;
		case JOY_DOWN:
			// at this configuration up & and down confused
			if(isCursorView){
				toogleCursorView();
			}
			yCursor += PIXEL_SIZE;
			if (yCursor >= SSD1306_HEIGHT){
				yCursor = 0;
			}
			break;
		case JOY_UP:
			// at this configuration up & and down confused
			if(isCursorView){
				toogleCursorView();
			}
			yCursor -= PIXEL_SIZE;
			if (yCursor < 0){
				yCursor = SSD1306_HEIGHT - PIXEL_SIZE;
			}
			break;

		case JOY_CENT:
			if (isCursorView) {
				toogleCursorView();
			}
			bufferHandle(EDITOR_BUFFER_BACKUP);
			viewMenu();
			return PROCESS_FINISHED;

		case BUTTON_A:
			ssd1306_DrawPixel(xCursor, yCursor, White);
			ssd1306_DrawPixel(xCursor + PIXEL_SIZE/2, yCursor, White);
			ssd1306_DrawPixel(xCursor, yCursor + PIXEL_SIZE/2, White);
			ssd1306_DrawPixel(xCursor + PIXEL_SIZE/2, yCursor + PIXEL_SIZE/2, White);
			break;

		case BUTTON_B:
			ssd1306_DrawPixel(xCursor, yCursor, Black);
			ssd1306_DrawPixel(xCursor + PIXEL_SIZE/2, yCursor, Black);
			ssd1306_DrawPixel(xCursor, yCursor + PIXEL_SIZE/2, Black);
			ssd1306_DrawPixel(xCursor + PIXEL_SIZE/2, yCursor + PIXEL_SIZE/2, Black);
			break;
		default:
			break;
	}

	static uint32_t timeLastCursorToogling = 0;
	if ((HAL_GetTick() - timeLastCursorToogling) > TIME_CURSOR_TOGGLING_MS) {
		timeLastCursorToogling = HAL_GetTick();
		toogleCursorView();
	}

	HAL_Delay(30);
	return PROCESS_EXECUTE;
}

static uint8_t editorBuffer[SSD1306_BUFFER_SIZE];

static void invertDrawInBuffer(){
	for (int i = 0; i < SSD1306_WIDTH * SSD1306_HEIGHT / 8; i++){
		editorBuffer[i] ^= 0xFF;
	}
}

static void bufferHandle(EditorBuffer_t handler){
	switch (handler) {
		case EDITOR_BUFFER_CLEAR:
			memset(editorBuffer, 0, SSD1306_BUFFER_SIZE);
			ssd1306_SetBuffer(editorBuffer);
			break;
		case EDITOR_BUFFER_BACKUP:
			memcpy(editorBuffer, ssd1306_GetBufferAddress(), SSD1306_BUFFER_SIZE);
			break;
		case EDITOR_BUFFER_SAVE:
			writeLastPictureInMem(editorBuffer);
			break;
		case EDITOR_BUFFER_LOAD:
			readLastPictureFromMem(editorBuffer);
			ssd1306_SetBuffer(editorBuffer);
			break;
		case EDITOR_BUFFER_RESUME:
			ssd1306_SetBuffer(editorBuffer);
			break;
	}
}

static void viewMenu(){
	ssd1306_Fill(Black);

	int xStart = 2, yStart = 2, fontH = 8, fontW = 6, fontX;
	fontX = xStart + fontW * 2;

	// draw triangle to LEFT
	ssd1306_DrawFilledTriangle(xStart, yStart + fontH / 2, xStart + fontH / 2, yStart, xStart + fontH / 2, yStart + fontH, White);
	ssd1306_SetCursor(fontX, yStart);
	ssd1306_WriteString("SAVE & EXIT", Font_6x8, White);

	yStart += (fontH + 2);
	// draw triangle to RIGHT
	ssd1306_DrawFilledTriangle(xStart + fontH / 2, yStart + fontH / 2, xStart, yStart, xStart, yStart + fontH, White);
	ssd1306_SetCursor(fontX, yStart);
	ssd1306_WriteString("CANCEL & EXIT", Font_6x8, White);

	// draw circle to CENTER
	yStart += (fontH + 2);
	int rad = fontH / 2 - 1;
	ssd1306_DrawFilledCircle(xStart + fontH / 2 - 1, yStart + fontH / 2, rad, White);
	ssd1306_SetCursor(fontX, yStart);
	ssd1306_WriteString("RESUME", Font_6x8, White);

	xStart--;
	yStart += (fontH + 2);
	// draw triangle to DOWN
	ssd1306_DrawFilledTriangle(xStart, yStart + fontH / 2, 		xStart + fontH / 2, yStart, 	xStart + fontH, yStart + fontH / 2, White);
	ssd1306_SetCursor(fontX, yStart);
	ssd1306_WriteString("CLEAR SCREEN", Font_6x8, White);

	yStart += (fontH + 2);
	// draw triangle to UP
	ssd1306_DrawFilledTriangle(xStart, yStart, 		xStart + fontH / 2, yStart + fontH / 2, 	xStart + fontH, yStart, White);
	ssd1306_SetCursor(fontX, yStart);
	ssd1306_WriteString("INVERT COLOR", Font_6x8, White);

	ssd1306_UpdateScreen();
}

ProcessState editorMenu(PushedButton handle){
	// delay need to blocking many buttons signals
	switch (handle) {
		case JOY_LEFT:
			//SAVE & EXIT
			bufferHandle(EDITOR_BUFFER_SAVE);
			return PROCESS_FINISHED;
		case JOY_RIGHT:
			return PROCESS_FINISHED;
		case JOY_CENT:
			bufferHandle(EDITOR_BUFFER_RESUME);
			ssd1306_UpdateScreen();
			return PROCESS_SWITCH_TO_PROCESS1;
		case JOY_DOWN:
			invertDrawInBuffer();
			bufferHandle(EDITOR_BUFFER_RESUME);
			ssd1306_UpdateScreen();
			return PROCESS_SWITCH_TO_PROCESS1;
		case JOY_UP:
			bufferHandle(EDITOR_BUFFER_CLEAR);
			ssd1306_UpdateScreen();
			return PROCESS_SWITCH_TO_PROCESS1;
			break;
		default:
			break;
	}
	return PROCESS_EXECUTE;
}
