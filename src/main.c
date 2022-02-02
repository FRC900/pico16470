#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "hardware/spi.h"
#include "imu.h"

static uint16_t buf[10] = {0};

void printBuf() {
    for (int i = 0; i < 10; i++) {
        printf("%5d ", buf[i]);
    }
    puts_raw("\r\n");
}

void loop() {
    /* Start DMA channels for burst read */
    IMU_Start_Burst(buf);
    IMU_Burst_Wait();

    printBuf();

    sleep_ms(1000);
}

int main()
{
    sleep_ms(1000 * 30);

    stdio_init_all();

    IMU_SPI_Init();

    while (true) {
        uint16_t prod_id = IMU_Read_Register(0x72);
        if (prod_id == 0x4256) {
            printf("Got expected PROD_ID:   0x%04x\r\n", prod_id);
            break;
        } else {
            printf("Got unexpected PROD_ID: 0x%04x\r\n", prod_id);
        }

        sleep_ms(10);
    }

    while (true) {
        loop();
    }
}
