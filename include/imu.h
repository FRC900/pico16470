#ifndef IMU_H_
#define IMU_H_

#include <stdint.h>

void IMU_SPI_Init();
uint16_t IMU_SPI_Transfer(uint16_t MOSI);
uint16_t IMU_Read_Register(uint8_t RegAddr);
uint16_t IMU_Write_Register(uint8_t RegAddr, uint8_t RegValue);
void IMU_Burst_Read_Blocking(uint16_t *buf);
void IMU_Reset();

void IMU_DMA_Burst_Read(uint16_t *buf);
void IMU_Hook_DR(void *callback);
void IMU_DMA_Burst_Wait();

#endif // IMU_H_
