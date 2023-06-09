# 硬禾寒假学堂5

## 题目详情
题目[详情](https://www.eetree.cn/project/detail/1352)

## 实现思路
1. 加速度部分使用IIC协议读取加速度计MMA7600FC，单开任务不断读取。使用循环限幅算法，设置阈值，检测静止。
2. 屏幕部分使用SPI协议发送屏幕数据，LCD数据芯片为ST7789，其颜色显示与手册不对应，可能是电路硬件的问题。
3. 彩灯部分调用LED PWM的API，使用标志位和任务计数器粗略定时，完成30s计数任务；同时将X,Y,Z轴的数据映射到LED彩灯的RGB三个硬件管脚的8位占空比上，实现LED灯随方向变化的功能。

## BUG
1. LCD部分颜色显示与芯片手册不对应，怀疑是LCD裸屏电路硬件管脚问题。
2. 彩灯PWM通道原先直连在串口0上，对于ESP32来说，串口0是不能定义为其他管脚的，直连在这里画硬件的人是不是傻逼？题目还要求控制这个彩灯。解决方案：使用飞线强制使用未使用的管脚，直连在LED上。

## 效果
[![Watch the video](https://raw.github.com/GabLeRoux/WebMole/master/ressources/WebMole_Youtube_Video.png)](./img/VID_20230313_190722.mp4)

## 提交的报告
[硬禾项目](%E7%A1%AC%E7%A6%BE%E9%A1%B9%E7%9B%AE.pdf)