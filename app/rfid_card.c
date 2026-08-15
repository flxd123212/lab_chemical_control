/*
 * RFID Card Reader Module Implementation
 * Hardware: RC522 RFID module via UART (/dev/ttySAC2)
 * Baud: 9600, 8N1
 * The RFID reader typically sends card ID as hex string terminated by \r\n
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "rfid_card.h"

static int rfid_fd = -1;

/* Whitelist */
static char whitelist[MAX_WHITELIST][MAX_CARD_LEN];
static int  whitelist_count = 0;

int rfid_init(void)
{
    struct termios tio;

    rfid_fd = open(RFID_DEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (rfid_fd < 0) {
        /* Try waiting a bit and retry */
        sleep(1);
        rfid_fd = open(RFID_DEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (rfid_fd < 0) {
            perror("open " RFID_DEVICE);
            return -1;
        }
    }

    /* Configure serial: 9600 8N1 */
    memset(&tio, 0, sizeof(tio));
    cfsetospeed(&tio, B9600);
    cfsetispeed(&tio, B9600);

    tio.c_cflag = CS8 | CREAD | CLOCAL;
    tio.c_iflag = IGNPAR | ICRNL;
    tio.c_oflag = 0;
    tio.c_lflag = 0;              /* raw mode, no echo, no signal */
    tio.c_cc[VMIN]  = 0;          /* non-blocking read */
    tio.c_cc[VTIME] = 0;

    tcflush(rfid_fd, TCIOFLUSH);
    tcsetattr(rfid_fd, TCSANOW, &tio);

    printf("RFID: %s opened at %d baud\n", RFID_DEVICE, RFID_BAUD);
    return 0;
}

void rfid_close(void)
{
    if (rfid_fd >= 0) {
        close(rfid_fd);
        rfid_fd = -1;
    }
}

int rfid_read_card(char *buf, int max_len)
{
    char tmp[64];
    int  n, i, j;

    if (rfid_fd < 0 || !buf || max_len <= 0)
        return -1;

    memset(tmp, 0, sizeof(tmp));
    n = read(rfid_fd, tmp, sizeof(tmp) - 1);
    if (n < 0) {
        if (errno == EAGAIN)
            return 0;  /* No data available */
        return -1;     /* Real error */
    }
    if (n == 0)
        return 0;      /* No data */

    /* Strip non-hex characters and spaces */
    j = 0;
    for (i = 0; i < n && j < max_len - 1; i++) {
        char c = tmp[i];
        if ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f')) {
            buf[j++] = c;
        }
    }
    buf[j] = '\0';

    if (j > 0) {
        printf("RFID: card detected -> %s\n", buf);
        return 1;  /* Card ID found */
    }

    return 0;
}

int rfid_verify_card(const char *card_id)
{
    int i;

    if (!card_id || card_id[0] == '\0')
        return 0;

    for (i = 0; i < whitelist_count; i++) {
        if (strcasecmp(whitelist[i], card_id) == 0) {
            printf("RFID: card %s VERIFIED (whitelist entry %d)\n", card_id, i);
            return 1;
        }
    }

    printf("RFID: card %s REJECTED (not in whitelist)\n", card_id);
    return 0;
}

int rfid_add_whitelist(const char *card_id)
{
    if (!card_id || card_id[0] == '\0')
        return -1;
    if (whitelist_count >= MAX_WHITELIST)
        return -1;

    /* Check for duplicates */
    for (int i = 0; i < whitelist_count; i++) {
        if (strcasecmp(whitelist[i], card_id) == 0)
            return 0; /* Already exists */
    }

    strncpy(whitelist[whitelist_count], card_id, MAX_CARD_LEN - 1);
    whitelist[whitelist_count][MAX_CARD_LEN - 1] = '\0';
    whitelist_count++;

    printf("RFID: added %s to whitelist (total: %d)\n", card_id, whitelist_count);
    return 0;
}

void rfid_load_default_whitelist(void)
{
    /* Pre-authorized card IDs (replace with your actual cards) */
    rfid_add_whitelist("A1B2C3D4");  /* Example card 1 - REPLACE ME */
    rfid_add_whitelist("E5F60708");  /* Example card 2 - REPLACE ME */
    rfid_add_whitelist("11223344");  /* Example card 3 - REPLACE ME */
}
