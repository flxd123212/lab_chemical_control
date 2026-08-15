#ifndef VOC21SENSOR_H
#define VOC21SENSOR_H

/**
 * voc21sensor.h - 21VOC五合一空气质量传感器 (甲醛 + CO2)
 *
 * 硬件连接: RX → GEC6818 RX2 (UART2: /dev/ttySAC2)
 * 通信参数: 9600 8N1, 主动上报模式
 *
 * 传感器协议 (主动上报, 每1-2秒一帧):
 *   帧格式 (9字节):
 *     Byte0: 0xAA      - 帧头
 *     Byte1: 0x15      - 帧类型 (空气质量数据)
 *     Byte2: HCHO_L    - 甲醛浓度低字节
 *     Byte3: HCHO_H    - 甲醛浓度高字节  (HCHO = (HCHO_H<<8 | HCHO_L) mg/m³)
 *     Byte4: CO2_L     - CO2浓度低字节
 *     Byte5: CO2_H     - CO2浓度高字节  (CO2 = (CO2_H<<8 | CO2_L) ppm)
 *     Byte6: TVOC_L    - TVOC浓度低字节 (保留, 本例未使用)
 *     Byte7: TVOC_H    - TVOC浓度高字节
 *     Byte8: CS        - 校验和 = 累加Byte0~Byte7 & 0xFF
 *
 * 如果实际传感器协议不同, 修改 voc21_parse_frame() 即可
 */
#define VOC21_FRAME_LEN  9
#define VOC21_BAUD       B9600
#define VOC21_DEVICE     "/dev/ttySAC2"

/* 甲醛报警阈值 (单位: mg/m³) */
#define HCHO_THRESHOLD   0.10f

/* CO2报警阈值 (单位: ppm) */
#define CO2_THRESHOLD    1000

void  voc21_init(void);
void  voc21_close(void);
int   voc21_read(float *hcho, float *co2);
/* 返回 0=成功, -1=失败; hcho/co2 输出测量值 */

#endif // VOC21SENSOR_H
