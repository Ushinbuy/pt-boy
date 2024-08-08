#include "flashMem.h"
#include "i2c.h"
#include "memory.h"
#include "createflags.h"
#include "aes.h"
#include <stdlib.h>

#define I2C_BUFFER_SIZE 0x30

typedef struct{
	volatile uint8_t isReceived;
	uint8_t data[I2C_BUFFER_SIZE];
} I2cMessage;

static I2cMessage i2cMessage;
static uint8_t rxcount = 0;
static uint8_t txcount=0;
static uint8_t *startPosition = 0;

char txBuffer[I2C_BUFFER_SIZE];

extern uint8_t flag1[FLAG1_LENGTH];
extern uint8_t flag2[FLAG2_LENGTH];
extern uint8_t key[KEY_LENGTH];

void HAL_I2C_ListenCpltCallback (I2C_HandleTypeDef *hi2c) {
	HAL_I2C_EnableListen_IT(hi2c);
}

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
	if (TransferDirection == I2C_DIRECTION_TRANSMIT)  // if the master wants to transmit the data
	{
		memset(i2cMessage.data, 0x0, I2C_BUFFER_SIZE);
		rxcount =0;
		HAL_I2C_Slave_Seq_Receive_IT(hi2c, i2cMessage.data+rxcount, 1, I2C_FIRST_FRAME);
	}

	else {
		txcount = 0;
		startPosition = (uint8_t*)&txBuffer[0];
		i2cMessage.data[0] = 0;  // Reset the start register as we have already copied it
		HAL_I2C_Slave_Seq_Transmit_IT(hi2c, startPosition+txcount, 1, I2C_FIRST_AND_LAST_FRAME);
	}
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (txcount >= I2C_BUFFER_SIZE){
		txcount = 0;
	}
	txcount++;
	HAL_I2C_Slave_Seq_Transmit_IT(hi2c, startPosition+txcount, 1, I2C_NEXT_FRAME);
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	rxcount++;
	if (rxcount < I2C_BUFFER_SIZE)
	{
		if (rxcount == I2C_BUFFER_SIZE-1)
		{
			HAL_I2C_Slave_Seq_Receive_IT(hi2c, i2cMessage.data+rxcount, 1, I2C_LAST_FRAME);
		}
		else
		{
			HAL_I2C_Slave_Seq_Receive_IT(hi2c, i2cMessage.data+rxcount, 1, I2C_NEXT_FRAME);
		}
	}

	if (rxcount == I2C_BUFFER_SIZE)
	{
		i2cMessage.isReceived = 1;
	}
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
	uint32_t errorcode = HAL_I2C_GetError(hi2c);

	if (errorcode == 4) { // AF error
		if (txcount == 0) { // error is while slave is receiving
			rxcount = 0;  // Reset the rxcount for the next operation
			i2cMessage.isReceived = 1;
		}
		else { // error while slave is transmitting
			txcount = 0;  // Reset the txcount for the next operation
		}
	}

	/* BERR Error commonly occurs during the Direction switch
	 * Here we the software reset bit is set by the HAL error handler
	 * Before resetting this bit, we make sure the I2C lines are released and the bus is free
	 * I am simply reinitializing the I2C to do so
	 */
	else if (errorcode == 1) { // BERR Error
		HAL_I2C_DeInit(hi2c);
		HAL_I2C_Init(hi2c);
		memset(i2cMessage.data,'\0',I2C_BUFFER_SIZE);  // reset the Rx buffer
		rxcount =0;  // reset the count
	}

	HAL_I2C_EnableListen_IT(hi2c);
}

void i2cHandle(uint8_t *isBizoneShow){
	if(i2cMessage.isReceived != 1){
		return;
	}
	i2cMessage.isReceived = 0;

	if(strncmp((char*)i2cMessage.data, "rabbit", 6) == 0){
		uint8_t newName[] = {
				i2cMessage.data[17],
				i2cMessage.data[26],
				i2cMessage.data[30],
				i2cMessage.data[10],
				i2cMessage.data[15],
				i2cMessage.data[11],
				i2cMessage.data[20],
				i2cMessage.data[29]
		};
		writeNameInMem(newName);
		taskSuccess(1);
	}
	else if(strncmp((char*)i2cMessage.data, "parrot", 6) == 0){
		memset(txBuffer, 0x0, I2C_BUFFER_SIZE);
		const uint8_t msgLength = 15;
		uint8_t outBuffer[16];
		memset(outBuffer, rand(), msgLength + 1);
		for(int i = 1; i < msgLength + 1; i++){
			uint8_t current_number = ((i ^ 0x75) * 3) & msgLength;
			if(current_number == 0){
				current_number = msgLength;
			}
			outBuffer[current_number] = key[i] ^ 0x3A;
		}
		memcpy(txBuffer, outBuffer, msgLength + 1);
	}
	else if(strncmp((char*)i2cMessage.data, "rat", 3) == 0){
		uint8_t encData[FLAG1_LENGTH];
		memcpy(encData, flag1, FLAG1_LENGTH);

		struct AES_ctx ctx;
		char lovelyConference[] = "__OFFZONE_2024__";

		uint8_t keyUsr[KEY_LENGTH];
		memcpy(keyUsr, key, KEY_LENGTH);

		uint8_t name[NAME_LENGTH];
		readLastNameFromMem(name);

		memcpy(&keyUsr[KEY_LENGTH - NAME_LENGTH], name, NAME_LENGTH);
		AES_init_ctx_iv(&ctx, keyUsr, (uint8_t*)lovelyConference);
		AES_CBC_encrypt_buffer(&ctx, encData, FLAG1_LENGTH);

		memset(txBuffer, 0x0, I2C_BUFFER_SIZE);
		memcpy(txBuffer, encData, FLAG1_LENGTH);
	}
	else if(strncmp((char*)i2cMessage.data, "bi.Zone", 7) == 0){
		*isBizoneShow ^= 1;
	}
	else{
		char outBuf[I2C_BUFFER_SIZE];
		getStatus(i2cMessage.data, outBuf);
		memset(txBuffer, 0x0, I2C_BUFFER_SIZE);
		memcpy(txBuffer, outBuf, I2C_BUFFER_SIZE);
	}
}
