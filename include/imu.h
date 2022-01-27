#ifndef IMU_H_
#define IMU_H_

#include <stdint.h>

void IMU_SPI_Init();
uint16_t IMU_SPI_Transfer(uint32_t MOSI);
uint16_t IMU_Read_Register(uint8_t RegAddr);
uint16_t IMU_Write_Register(uint8_t RegAddr, uint8_t RegValue);
void IMU_Read_Burst(uint16_t *buf);
void IMU_Start_Burst(uint16_t *buf);
void IMU_Finish_Burst();
void IMU_Reset();

#endif // IMU_H_
