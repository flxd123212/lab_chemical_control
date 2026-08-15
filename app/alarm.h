/*
 * Alarm Module - Buzzer, LED Indicators, Relay Control
 * Hardware:
 *   - Buzzer: /dev/pwm (PWM2, GPIO C+14)
 *   - LEDs:   /dev/Led (6_9_led_driver)
 *       LED1(GPIOE13) = Red LED  (D7)
 *       LED2(GPIOC17) = Green LED (D8)
 *       LED3(GPIOC8)  = D9
 *       LED4(GPIOC7)  = D10
 *   - Relay: /dev/relay (GPIO B+28)
 *
 * ioctl protocol (via 6_9_led_driver):
 *   ioctl(fd, LEDx, 0) = ON, ioctl(fd, LEDx, 1) = OFF
 */

#ifndef ALARM_H
#define ALARM_H

/* PWM IOCTL commands */
#define PWM_IOCTL_SET_FREQ  1
#define PWM_IOCTL_STOP      0

/* LED driver ioctl (from 6_9_led_driver /dev/Led) */
#define LED_MAGIC  'x'
#define LED1  _IO(LED_MAGIC, 0)   /* GPIOE13 = Red LED  (D7) */
#define LED2  _IO(LED_MAGIC, 1)   /* GPIOC17 = Green LED (D8) */
#define LED3  _IO(LED_MAGIC, 2)   /* GPIOC8  = D9 */
#define LED4  _IO(LED_MAGIC, 3)   /* GPIOC7  = D10 */

int  alarm_init(void);
void alarm_close(void);

/* Buzzer control */
void alarm_beep_on(int freq_hz);
void alarm_beep_off(void);

/* LED control (via /dev/Led) */
void alarm_red_led_on(void);
void alarm_red_led_off(void);
void alarm_green_led_on(void);
void alarm_green_led_off(void);

/* Relay control (for door / gas valve / fan) */
void alarm_relay_on(void);
void alarm_relay_off(void);

/* Combined alarm actions */
void alarm_activate(void);     /* Full alarm: red LED + buzzer + relay */
void alarm_deactivate(void);   /* Stop alarm, restore normal state */
void alarm_indicate_normal(void);  /* Green LED on, rest off */
void alarm_indicate_door_open(void); /* Flash pattern for door access */

#endif /* ALARM_H */
