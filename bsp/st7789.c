#include <string.h>
#include <inttypes.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"

#include "font.h"
#include "st7789.h"
#include "mma7600fc.h"

#define TAG "ST7789"
#define	_DEBUG_ 0

#define SPI_CS_PIN 13
#define PIN_CS_BIT ((1ULL << SPI_CS_PIN))
#define SPI_DC_PIN 17
#define PIN_DC_BIT ((1ULL << SPI_DC_PIN))
#define SPI_SCL_PIN 41
#define PIN_SCL_BIT ((1ULL << SPI_SCL_PIN))
#define SPI_MOSI_PIN 21
#define PIN_MOSI_BIT ((1ULL << SPI_MOSI_PIN))
#define SPI_RST_PIN 18
#define PIN_RST_BIT ((1ULL << SPI_RST_PIN))

#define LCD_WIDTH 128
#define LCD_HEIHGT 128

uint8_t x_offset = 2;
uint8_t y_offset = 1;
uint16_t POINT_COLOR = 0x0000;	/* 画笔颜色 */
uint16_t BACK_COLOR = 0xFFFF;  	/* 背景色 */ 

spi_device_handle_t spi;
static const int SPI_Frequency = SPI_MASTER_FREQ_20M;

/*************************************基础SPI定义**************************************/
void spi_master_init()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = PIN_CS_BIT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;

    gpio_config(&io_conf);
    gpio_set_level( SPI_CS_PIN, 0);

    io_conf.pin_bit_mask = PIN_DC_BIT;
    gpio_config(&io_conf);
    gpio_set_level( SPI_DC_PIN, 0);

    io_conf.pin_bit_mask = PIN_RST_BIT;
    gpio_config(&io_conf);
    gpio_set_level( SPI_RST_PIN, 0);

	esp_err_t ret;
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,   // MISO信号线
        .mosi_io_num = SPI_MOSI_PIN,   // MOSI信号线
        .sclk_io_num = SPI_SCL_PIN,    // SCLK信号线
        .quadwp_io_num = -1,       // WP信号线，专用于QSPI的D2
        .quadhd_io_num = -1,       // HD信号线，专用于QSPI的D3
        .max_transfer_sz = 2048, // 最大传输数据大小

    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_Frequency,    // Clock out
        .mode = 0,                          // SPI mode 0
        .queue_size = 7,                    // We want to be able to queue 7 transactions at a time
        // .pre_cb=spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };
    // Initialize the SPI bus
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, 0);
    ESP_ERROR_CHECK(ret);
    // Attach the LCD to the SPI bus
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
}

static void spi_send_cmd(uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t;
    gpio_set_level(SPI_DC_PIN, 0);
    gpio_set_level(SPI_CS_PIN, 0);
    memset(&t, 0, sizeof(t)); 
    t.length = 8;                               // Command is 8 bits
    t.tx_buffer = &cmd;                         // The data is the cmd itself
    t.user = (void *)0;                         // D/C needs to be set to 0
    ret = spi_device_polling_transmit(spi, &t); // Transmit!
    gpio_set_level(SPI_CS_PIN, 1);
    ESP_ERROR_CHECK(ret);
}

static void spi_send_data(uint8_t data)
{
    esp_err_t ret;
    spi_transaction_t t;
    gpio_set_level(SPI_DC_PIN, 1);
    gpio_set_level(SPI_CS_PIN, 0);
    memset(&t, 0, sizeof(t));                   // Zero out the transaction
    t.length = 8;                               // Len is in bytes, transaction length is in bits.
    t.tx_buffer = &data;                        // Data
    t.user = (void *)1;                         // D/C needs to be set to 1
    ret = spi_device_polling_transmit(spi, &t); // Transmit!
    gpio_set_level(SPI_CS_PIN, 1);
    ESP_ERROR_CHECK(ret);
}

/*************************************基础驱动**************************************/
static void st7789_config()
{
    gpio_set_level(SPI_CS_PIN, 1);

    //rst
    gpio_set_level(SPI_RST_PIN, 1);
    vTaskDelay(120 / portTICK_PERIOD_MS);
    gpio_set_level(SPI_RST_PIN, 0);
    vTaskDelay(120 / portTICK_PERIOD_MS);
    gpio_set_level(SPI_RST_PIN, 1);
    vTaskDelay(120 / portTICK_PERIOD_MS);

    spi_send_cmd(0x11);     //sleep out
    vTaskDelay(200 / portTICK_PERIOD_MS);

    spi_send_cmd(0x36);     //刷新方向和显示方向等
    spi_send_data(0x00);    //一切按正常思维走

    spi_send_cmd(0x3A);     //颜色格式
    spi_send_data(0x65);    

    spi_send_cmd(0xB2);     //porch设置
    spi_send_data(0x0C); 
    spi_send_data(0x0C); 
    spi_send_data(0x00);    
    spi_send_data(0x33);
    spi_send_data(0x33); 

    spi_send_cmd(0xB7);     //GATE CONTROL
    spi_send_data(0x72);    

    spi_send_cmd(0xBB);     //VCOM CONTROL
    spi_send_data(0x3D);   

    spi_send_cmd(0x0C);     //LCM CONTROL
    spi_send_data(0x2C);   

    spi_send_cmd(0xC2);
    spi_send_data(0x01);
    spi_send_data(0xFF);    //VDVVRHEN: VDV and VRH Command Enable

    spi_send_cmd(0xC3);
    spi_send_data(0x19);    //VRHS: VRH Set

    spi_send_cmd(0xC4);
    spi_send_data(0x20);    //VDVS: VDV Set

    spi_send_cmd(0xC6);
    spi_send_data(0x0F);    //FRCTRL2: Frame Rate control in normal mode

    spi_send_cmd(0xD0);
    spi_send_data(0xA4);
    spi_send_data(0xA1);    //PWCTRL1: Power Control 1

    spi_send_cmd(0xE0);     //Positive Voltage Gamma Control
    spi_send_data(0xD0);
    spi_send_data(0x04);
    spi_send_data(0x0D);
    spi_send_data(0x11);
    spi_send_data(0x13);
    spi_send_data(0x2B);
    spi_send_data(0x3F);
    spi_send_data(0x54);
    spi_send_data(0x4C);
    spi_send_data(0x18);
    spi_send_data(0x0D);
    spi_send_data(0x0B);
    spi_send_data(0x1F);
    spi_send_data(0x23);
 
    spi_send_cmd(0xE1);     //Negative Voltage Gamma Control
    spi_send_data(0xD0);
    spi_send_data(0x04);
    spi_send_data(0x0C);
    spi_send_data(0x11);
    spi_send_data(0x13);
    spi_send_data(0x2C);
    spi_send_data(0x3F);
    spi_send_data(0x44);
    spi_send_data(0x51);
    spi_send_data(0x2F);
    spi_send_data(0x1F);
    spi_send_data(0x1F);
    spi_send_data(0x20);
    spi_send_data(0x23);

    spi_send_cmd(0x21);     //反向
    spi_send_cmd(0x29);     //display on 
}

void screen_init()
{
    spi_master_init();
    st7789_config();
}

void lcd_set_address(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    //配置偏移量
    x1+=x_offset;
    y1+=y_offset;
    x2+=x_offset;
    y2+=y_offset;

    spi_send_cmd(0x2a);
    spi_send_data(x1 >> 8);
    spi_send_data(x1);
    spi_send_data(x2 >> 8);
    spi_send_data(x2);

    spi_send_cmd(0x2b);
    spi_send_data(y1 >> 8);
    spi_send_data(y1);
    spi_send_data(y2 >> 8);
    spi_send_data(y2);
}

void lcd_display_on(void)
{
	spi_send_cmd(0x29);
}

void lcd_display_off(void)
{
	spi_send_cmd(0x28);
}

void lcd_write_ram(void)
{
	spi_send_cmd(0x2C);
}

void lcd_clear(uint16_t color)
{	
    uint16_t i, j;

	lcd_display_off();		/* 关闭显示 */
	lcd_set_address(0, 0,LCD_WIDTH - 1, LCD_HEIHGT - 1);
	lcd_write_ram();
	
	for(i = 0; i < LCD_HEIHGT; i++)
	{
		for(j = 0; j < LCD_WIDTH; j++)
		{
			spi_send_data(color >> 8);
			spi_send_data(color & 0x00ff);
		}
	}
	lcd_display_on();		/* 打开显示 */
}

/*************************************基础UI**************************************/
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    if(x>=LCD_WIDTH || y>= LCD_HEIHGT)
    {
        ESP_LOGW(TAG,"超限");
        return;
    }
	lcd_set_address(x, y, x, y);
	lcd_write_ram();
	spi_send_data(color >> 8);
    spi_send_data(color & 0x00ff); 
}

/**************************************************************
函数名称 : lcd_show_char
函数功能 : lcd显示一个字符
输入参数 : x,y:起始坐标
		   num:要显示的字符:" "--->"~"
		   size:字体大小 16/24/32
		   mode:叠加方式(1)还是非叠加方式(0)
返回值  	 : 无
备注		 : 无
**************************************************************/
void lcd_show_char(uint16_t x, uint16_t y,char num,uint8_t size,uint8_t mode)
{ 
    uint16_t temp, t1, t;
	uint16_t y0 = y;
	uint16_t csize = ((size / 8) + ((size % 8) ? 1 : 0)) * (size/2);	/* 得到字体一个字符对应点阵集所占的字节数	 */
	
 	num = (uint8_t)num - ' ';	/* 得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库） */
	
	for(t = 0; t < csize; t++)	/*遍历打印所有像素点到LCD */
	{   
     	if(16 == size)
		{
			temp = asc2_1608[(uint8_t)num][t];	/* 调用1608字体 */
     	}
		else if(24 == size)
		{
			temp = asc2_2412[(uint8_t)num][t];	/* 调用2412字体 */
		}
		else if(32 == size)
		{
			temp = asc2_3216[(uint8_t)num][t];	/* 调用3216数码管字体 */
		}
		else
		{	
            ESP_LOGE(TAG,"字体大小不对");
			return;		/* 没有找到对应的字库 */
		}
		for(t1 = 0; t1 < 8; t1++)	/* 打印一个像素点到液晶 */
		{			    
			if(temp & 0x80)
			{
				lcd_draw_point(x, y, POINT_COLOR);
			}
			else if(0 == mode)
			{
				lcd_draw_point(x, y, BACK_COLOR);
			}
			temp <<= 1;
			y++;
			
			if(y >= LCD_HEIHGT)
			{
                ESP_LOGE(TAG,"超限");
				return;		/* 超区域了 */
			}
			if((y - y0) == size)
			{
				y = y0;
				x++;
				if(x >= LCD_WIDTH)
				{
                    ESP_LOGE(TAG,"超限");
					return;	/* 超区域了 */
				}
				break;
			}
		}  	 
	}  	    	   	 	  
} 

/**************************************************************
函数名称 : lcd_show_string
函数功能 : lcd显示字符串
输入参数 : x,y:起始坐标
		   width,height：区域大小
		   *p:字符串起始地址
		   size:字体大小
		   mode:叠加方式(1)还是非叠加方式(0)
返回值  	 : 无
备注		 : 无
**************************************************************/
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height,char *p,uint8_t size,uint8_t mode)
{
	uint16_t x0 = x;
	width += x;
	height += y;
	
    while(*p != '\0')		/* 判断是不是非法字符! */
    {       
        if(x >= width || *p == '\n' || *p == '\t' )
		{
			x = x0; 
			y += size;
            p++;
            continue;
		}
		
        if(y >= height)
		{	
			break;
        }
        lcd_show_char(x, y, *p, size, mode);
        x += size/2;
        p++;
    }  
}

/**************************************************************
函数名称 : lcd_show_chinese
函数功能 : lcd显示字符串
输入参数 : x,y:起始坐标
		   *ch:汉字字符串起始地址
		   size:字体大小
		   n:汉字个数
		   mode:叠加方式(1)还是非叠加方式(0)
返回值  	 : 无
备注		 : 无
**************************************************************/
void lcd_show_chinese(uint16_t x, uint16_t y, const uint8_t * ch,uint8_t size,uint8_t n,uint8_t mode)
{
	uint32_t temp, t, t1;
    uint16_t y0 = y;
    uint32_t csize = ((size / 8) + ((size % 8) ? 1 : 0)) * (size) * n;	/* 得到字体字符对应点阵集所占的字节数 */
    
    for(t = 0; t < csize; t++)
	{   												   
		temp = ch[t];	/* 得到点阵数据 */
		
		for(t1 = 0; t1 < 8; t1++)
		{
			if(temp & 0x80)
			{
				lcd_draw_point(x, y, POINT_COLOR);
			}
			else if(mode==0)
			{
				lcd_draw_point(x, y, BACK_COLOR);
			}
			temp <<= 1;
			y++;
			if((y - y0) == size)
			{
				y = y0;
				x++;
				break;
			}
		}  	 
	}  
}

TaskHandle_t lcdTask_Handle = NULL;
#define LCD_TASK_HEAP 10000

static void lcd_task(void *arg)
{
    int8_t x = 0,y = 0,z = 0;
    uint8_t shake = 0;
    char show_buffer[128]={'\0'};

    while (1)
    {
        x=get_mma7600fc_x();
        y=get_mma7600fc_y();
        z=get_mma7600fc_z();
        shake = get_mma7600fc_shake();

        sprintf(show_buffer,"X-axis=%d \nY-axis=%d \nZ-axis=%d \n",x,y,z);
        lcd_show_string(0, 0, 127, 127,show_buffer,16,0);

        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    
}
void lcd_task_init()
{
    screen_init();
    lcd_clear(WHITE);

    xTaskCreatePinnedToCore(lcd_task, "screen", LCD_TASK_HEAP, NULL, 3, lcdTask_Handle, 0);
}