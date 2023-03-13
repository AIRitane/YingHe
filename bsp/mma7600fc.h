#ifndef __MMA7600FC_H
#define __MMA7600FC_H

#define MMA7660_XOUT  0x00   // 6-bit output value X 
#define MMA7660_YOUT  0x01   // 6-bit output value Y 
#define MMA7660_ZOUT  0x02   // 6-bit output value Z
#define MMA7660_TILT  0x03   // Tilt Status 
#define MMA7660_SRST  0x04   // Sampling Rate Status
#define MMA7660_SPCNT 0x05   // Sleep Count
#define MMA7660_INTSU 0x06   // Interrupt Setup
#define MMA7660_MODE  0x07   // Mode
#define MMA7660_SR    0x08   // Auto-Wake/Sleep and 
                      		 // Portrait/Landscape samples 
                      		 // per seconds and Debounce Filter
#define MMA7660_PDET  0x09   // Tap Detection
#define MMA7660_PD    0x0A   // Tap Debounce Count
 
 
//=========MMA7660 功能参数==================//
#define MMA7660_DEV_ADDR   0x4C //Normally,can range 0x08 to 0xEF


void MMA7660_Begin(void);
uint8_t MMA7660_GetResult(uint8_t Regs_Addr);
void mma_task_init();
int8_t get_mma7600fc_x();
int8_t get_mma7600fc_y();
int8_t get_mma7600fc_z();
uint8_t get_mma7600fc_shake();
void clear_mma7600fc_shake();

#endif