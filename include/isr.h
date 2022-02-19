#ifndef ISR_H_
#define ISR_H_

/* Header includes require for prototypes */
#include <stdint.h>

void ISR_Start_IMU_Burst();
void ISR_Finish_IMU_Burst();

/* Public variables exported from module */
extern volatile uint32_t g_wordsPerCapture;
extern volatile uint32_t g_captureInProgress;
extern volatile uint32_t g_spi_rx_upper;

#endif // ISR_H_
