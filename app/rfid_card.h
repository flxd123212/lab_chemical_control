/*
 * RFID Card Reader Module
 * Hardware: RC522 RFID module via UART (/dev/ttySAC2)
 * Typically outputs card ID as hex string (e.g. "A1B2C3D4")
 */

#ifndef RFID_CARD_H
#define RFID_CARD_H

#define RFID_DEVICE   "/dev/ttySAC2"
#define RFID_BAUD     9600
#define MAX_CARD_LEN  32
#define MAX_WHITELIST 64

/* Card ID as printable string */
typedef struct {
    char card_id[MAX_CARD_LEN];
    int  verified;   /* 0 = unknown, 1 = authorized */
} card_info_t;

int  rfid_init(void);
void rfid_close(void);

/* Read a card ID (non-blocking). Returns 1 if card detected, 0 if not, -1 on error. */
int  rfid_read_card(char *buf, int max_len);

/* Verify card against whitelist. Returns 1 if authorized, 0 if not. */
int  rfid_verify_card(const char *card_id);

/* Add card to whitelist (persistent in memory during runtime) */
int  rfid_add_whitelist(const char *card_id);

/* Load default whitelist entries */
void rfid_load_default_whitelist(void);

#endif /* RFID_CARD_H */
