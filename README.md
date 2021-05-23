# STM32_Tempalte

## 注意点
- sdram使用cube生成后需要新初始化序列才能使用
- sdram的刷新频率计数器需要注意
可以看cube的时钟树确定FMC的主频
刷新频率计数器(以SDCLK频率计数),计算方法:
COUNT=SDRAM刷新周期/行数-20=SDRAM刷新周期(us)SDCLK频率(Mhz)/行数
- LTCD的时钟需要注意