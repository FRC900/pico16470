#ifndef IMU_H_
#define IMU_H_

#include <stdint.h>

void IMU_SPI_Init();
uint16_t IMU_SPI_Transfer(uint8_t MOSI);
uint16_t IMU_Read_Register(uint8_t RegAddr);
uint16_t IMU_Write_Register(uint8_t RegAddr, uint8_t RegValue);
void IMU_Update_SPI_Config();
void EnableImuSpiDMA();
void IMU_Disable_SPI_DMA();
void IMU_Start_Burst(uint8_t * bufEntry);
void IMU_Reset();

#endif // IMU_H_
