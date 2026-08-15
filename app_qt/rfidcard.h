#ifndef RFIDCARD_H
#define RFIDCARD_H

#define MAX_CARD_LEN 32

/**
 * RFID刷卡模块
 * 设备: /dev/ttySAC1 (RX1), 9600 8N1
 */
void rfid_init(void);
void rfid_close(void);
void rfid_load_default_whitelist(void);
int  rfid_read_card(char *buf, int buf_size);
int  rfid_verify_card(const char *card_id);

#endif // RFIDCARD_H
