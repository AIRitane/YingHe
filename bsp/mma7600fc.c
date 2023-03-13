#include <stdio.h>
#include <string.h>
#include "mma7600fc.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 3
#define I2C_FREQ_HZ 50000
#define ESP_SLAVE_ADDR MMA7660_DEV_ADDR
#define ACK_CHECK_EN 0x1  /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0 /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0       /*!< I2C ack value */
#define NACK_VAL 0x1

#define TAG "MMA7600FS"
/*
 **********************************************************
 *
 * iic相关函数
 *
 **********************************************************
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = 0;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN, // select GPIO specific to your project
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_SCL_PIN, // select GPIO specific to your project
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ, // select frequency specific to your project
        // .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
    };

    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

void MMA7660_WriteReg(uint8_t Regs_Addr, uint8_t Regs_Data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MMA7660_DEV_ADDR << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, Regs_Addr, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, Regs_Data, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_master_write_slave error\n");
    }
}

uint8_t MMA7660_ReadReg(uint8_t Regs_Addr)
{
    uint8_t data;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MMA7660_DEV_ADDR << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, Regs_Addr, ACK_CHECK_EN);

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (MMA7660_DEV_ADDR << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &data, NACK_VAL);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_master_read_slave error !!\n");
    }

    return data;
}

uint8_t MMA7660_GetResult(uint8_t Regs_Addr)
{
    uint8_t ret;
    uint32_t count = 0;

    // 等待转换完成并取出值
    ret = MMA7660_ReadReg(Regs_Addr);

    while (ret & 0x40)
    {
        count++;
        if (count == 100)
        {
            ESP_LOGE(TAG, "获取速度失败，请重新检查");
            return -1;
        }
        ret = MMA7660_ReadReg(Regs_Addr); // 数据更新，重新读
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    return ret;
}

void MMA7660_Begin(void)
{
    i2c_master_init();
    MMA7660_WriteReg(MMA7660_MODE, 0x00);  // standby mode	//首先必须进入standby模式才能更改寄存器
    MMA7660_WriteReg(MMA7660_SPCNT, 0x00); // No Sleep Count
    MMA7660_WriteReg(MMA7660_INTSU, 0x00); // 没中断
    MMA7660_WriteReg(MMA7660_PDET, 0xE0);  // 关闭撞击检测
    MMA7660_WriteReg(MMA7660_SR, 0x02);    // orginal value: 0x34				//8 samples/s,TILT debounce filter=2
    MMA7660_WriteReg(MMA7660_PD, 0x00);    // original value: 0x00			//No tap detection debounce count enabled
    MMA7660_WriteReg(MMA7660_MODE, 0x01);  // original value: 0x41			//Active Mode,INT= push-pull and active low
}


TaskHandle_t mmaTask_Handle = NULL;
#define MMA_TASK_HEAP 2048
typedef struct
{
    int8_t x;
    int8_t y;
    int8_t z;
    uint8_t shake;
}mma7600fc_t;

mma7600fc_t mma7600fc;

int8_t get_mma7600fc_x()
{
    return mma7600fc.x;
}

int8_t get_mma7600fc_y()
{
    return mma7600fc.y;
}

int8_t get_mma7600fc_z()
{
    return mma7600fc.z;
}

uint8_t get_mma7600fc_shake()
{
    return mma7600fc.shake;
}

void clear_mma7600fc_shake()
{
    mma7600fc.shake = 0;
}

#define SHAKE_DELTA 3

//循环限幅
//31 32 0 1 2
int loop_caculate(int Input,int minValue,int maxValue)
{
    int len;
    if (maxValue < minValue)
    {
        return Input;
    }

    if (Input >= maxValue)
    {
        len = maxValue - minValue;
        while (Input >= maxValue)
        {
            Input -= len;
        }
    }
    else if (Input <= minValue)
    {
        len = maxValue - minValue;
        while (Input <= minValue)
        {
            Input += len;
        }
    }
    return Input;
}

static void mma_task(void *arg)
{
    int8_t pr_x = 0,pr_y = 0,pr_z = 0;
    int8_t del_x = 0,del_y = 0,del_z = 0;
    while (1)
    {
        mma7600fc.x=(int)(MMA7660_GetResult(MMA7660_XOUT)&0x3f)-32;
        mma7600fc.y=(int)(MMA7660_GetResult(MMA7660_YOUT)&0x3f)-32;
        mma7600fc.z=(int)(MMA7660_GetResult(MMA7660_ZOUT)&0x3f)-32;

        //震动检测
        del_x = loop_caculate(((int)mma7600fc.x - pr_x),-32,31);
        del_y = loop_caculate(((int)mma7600fc.y - pr_y),-32,31);
        del_z = loop_caculate(((int)mma7600fc.z - pr_z),-32,31);
        if(del_x > SHAKE_DELTA || del_y > SHAKE_DELTA || del_z > SHAKE_DELTA)
        {
            mma7600fc.shake = 1;
        }
        pr_x = mma7600fc.x;
        pr_y = mma7600fc.y;
        pr_z = mma7600fc.z;


        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
}
void mma_task_init()
{
    MMA7660_Begin();

    xTaskCreatePinnedToCore(mma_task, "lcd_task", MMA_TASK_HEAP, NULL, 10, mmaTask_Handle, 1);
}
