#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "led.h"
#include "math.h"
#include "mma7600fc.h"

#define TAG "LED"

#define LED_B_PIN 2
#define LED_G_PIN 1
#define LED_R_PIN 12

void led_init()
{
     ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT,  // resolution of PWM duty
        .freq_hz = 5000,                          // frequency of PWM signal
        .speed_mode = LEDC_LOW_SPEED_MODE,     // timer mode
        .timer_num = LEDC_TIMER_0,             // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    ledc_timer_config(&ledc_timer);

    ledc_timer.duty_resolution = LEDC_TIMER_8_BIT,
    ledc_timer_config(&ledc_timer);

    ledc_timer.duty_resolution = LEDC_TIMER_8_BIT,
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel[3] = {
        {
            .channel    = LEDC_CHANNEL_0,
            .duty       = 128,
            .gpio_num   = LED_R_PIN,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0
        },
        {
            .channel    = LEDC_CHANNEL_1,
            .duty       = 128,
            .gpio_num   = LED_B_PIN,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0
        },
        {
            .channel    = LEDC_CHANNEL_2,
            .duty       = 128,
            .gpio_num   = LED_G_PIN,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0
        },
    };

    for (int  ch = 0; ch < 3; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }
}

void set_color(int8_t r,int8_t g,int8_t b)
{
    uint8_t _r = abs(r)*255/32;
    uint8_t _g = abs(g)*255/32;
    uint8_t _b = abs(b)*255/32;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, _r);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, _g);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, _b);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
}


TaskHandle_t ledTask_Handle = NULL;
#define LED_TASK_HEAP 2048

void led_loop()
{
    static uint8_t x=0,y=255,z=125;
    set_color(x,y,z);
    x++;y++;z++;
}

static void led_task(void *arg)
{
    uint32_t count = 0;
    int8_t x = 0,y = 0,z = 0;
    while (1)
    {
        count++;
        x=get_mma7600fc_x();
        y=get_mma7600fc_y();
        z=get_mma7600fc_z();

        if(get_mma7600fc_shake())
        {
            count = 0;
            clear_mma7600fc_shake();
        }

        if (count >= 30000/20)
        {
            led_loop();
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
        
        else
        {
            set_color(x,y,z);
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    
}
void led_task_init()
{
    led_init();

    xTaskCreatePinnedToCore(led_task, "LED", LED_TASK_HEAP, NULL, 10, ledTask_Handle, 1);
}

