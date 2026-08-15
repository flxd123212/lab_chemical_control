#ifndef GASMONITOR_H
#define GASMONITOR_H

/**
 * 气体传感器 (MQ-2)
 * 设备: /dev/gec_gas_drv
 * read() 返回 '0'=正常, '1'=报警
 */
void gas_init(void);
void gas_close(void);
int  gas_read(void);    // 0=正常, 1=报警, -1=错误

#endif // GASMONITOR_H
