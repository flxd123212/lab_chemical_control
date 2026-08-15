#ifndef ALARM_H
#define ALARM_H

/**
 * 声光报警模块
 *  - 蜂鸣器: /dev/pwm, ioctl(fd, 1, freq) 开, ioctl(fd, 0, 0) 关
 *  - 红色LED (D7): /dev/Led, ioctl(fd, LED1, 0) 亮, ioctl(fd, LED1, 1) 灭
 *  - 绿色LED (D8): /dev/Led, ioctl(fd, LED2, 0) 亮, ioctl(fd, LED2, 1) 灭
 *  - 继电器: /dev/relay, ioctl(fd, 1, 0) 开, ioctl(fd, 0, 0) 关
 *
 * LED ioctl 定义 (来自 6_9_led_driver):
 *   LED1 = GPIOE13, LED2 = GPIOC17, LED3 = GPIOC8, LED4 = GPIOC7
 *   arg=0 → ON, arg=1 → OFF
 */

/* LED driver ioctl */
#define LED_MAGIC  'x'
#define LED1  _IO(LED_MAGIC, 0)   /* GPIOE13 = Red LED  (D7) */
#define LED2  _IO(LED_MAGIC, 1)   /* GPIOC17 = Green LED (D8) */
#define LED3  _IO(LED_MAGIC, 2)   /* GPIOC8  = D9 */
#define LED4  _IO(LED_MAGIC, 3)   /* GPIOC7  = D10 */

void alarm_init(void);
void alarm_close(void);
void alarm_activate(void);      // 触发全部报警
void alarm_deactivate(void);    // 解除全部报警
void alarm_relay_on(void);      // 继电器开 (开门)
void alarm_relay_off(void);     // 继电器关 (关门)
void alarm_indicate_door_open(void);  // 绿灯闪烁
void alarm_red_led_on(void);
void alarm_red_led_off(void);
void alarm_green_led_on(void);
void alarm_green_led_off(void);

#endif // ALARM_H
