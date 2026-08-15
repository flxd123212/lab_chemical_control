/*
 * Gas Sensor Monitor Module
 * Hardware: MQ-2 gas/smoke sensor via GPIO
 * Device: /dev/gec_gas_drv
 * Read returns: '0' = normal, '1' = alarm
 */

#ifndef GAS_MONITOR_H
#define GAS_MONITOR_H

#define GAS_DEVICE  "/dev/gec_gas_drv"

int  gas_init(void);
void gas_close(void);

/* Read gas sensor. Returns 0 = normal, 1 = alarm, -1 = error. */
int  gas_read(void);

#endif /* GAS_MONITOR_H */
