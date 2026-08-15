#ifndef WEATHER_H
#define WEATHER_H

/**
 * 温湿度传感器 (DHT11) - 实际设备名
 * 设备: /dev/ttySAC3 (UART-based, DHT11 on RX3)
 * read() 返回4字节数据，格式参考具体驱动
 */
void weather_init(void);
void weather_close(void);
int  weather_read(float *temperature, float *humidity);

#endif // WEATHER_H
