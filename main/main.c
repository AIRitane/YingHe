#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "mma7600fc.h"
#include "st7789.c"
#include "led.h"

void app_main(void)
{
    printf("Hello world!\n");
    mma_task_init();
    led_task_init();
    lcd_task_init();
    while(1){
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

