/**
 * rfid_dump.c - RFID卡号读取调试工具
 *
 * 使用方法:
 *   1. 交叉编译: arm-linux-gnueabihf-gcc rfid_dump.c -o rfid_dump
 *   2. 传到 GEC6818 板子
 *   3. 运行: ./rfid_dump
 *   4. 将RFID卡靠近读卡器, 终端会显示卡号
 *   5. 记下显示的卡号, 加到 rfidcard.c 的白名单 whitelist[] 中
 *
 * 硬件连接: RFID RX → GEC6818 RX2 (/dev/ttySAC2)
 * 通信参数: 9600 8N1
 *
 * 按 Ctrl+C 退出
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>

#define RFID_DEVICE  "/dev/ttySAC2"
#define RFID_BAUD    B9600
#define BUF_SIZE     64

static volatile int keep_running = 1;

void sigint_handler(int sig)
{
    (void)sig;
    keep_running = 0;
}

/* 将原始字节转成十六进制字符串显示 */
static void print_hex(const unsigned char *data, int len)
{
    for (int i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
}

/* 尝试将数据作为ASCII字符串显示 (过滤不可见字符) */
static void print_ascii(const unsigned char *data, int len)
{
    for (int i = 0; i < len; i++) {
        char c = (char)data[i];
        if (c >= 0x20 && c <= 0x7E)
            putchar(c);
    }
}

int main(void)
{
    signal(SIGINT, sigint_handler);

    printf("============================================\n");
    printf("  RFID Card UID Dumper\n");
    printf("  Device: %s\n", RFID_DEVICE);
    printf("  Baud:   9600 8N1\n");
    printf("============================================\n");
    printf("  Please tap your RFID card on the reader...\n");
    printf("  Press Ctrl+C to exit.\n");
    printf("============================================\n\n");

    int fd = open(RFID_DEVICE, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("ERROR: Failed to open " RFID_DEVICE);
        printf("\n  Tips:\n");
        printf("  - Make sure RFID is connected to RX2\n");
        printf("  - Check device path: %s\n", RFID_DEVICE);
        printf("  - Run with: sudo ./rfid_dump  (if permission denied)\n");
        return 1;
    }

    /* 设置串口参数 */
    struct termios options;
    tcgetattr(fd, &options);

    cfsetispeed(&options, RFID_BAUD);
    cfsetospeed(&options, RFID_BAUD);
    options.c_cflag &= ~PARENB;     /* No parity */
    options.c_cflag &= ~CSTOPB;     /* 1 stop bit */
    options.c_cflag &= ~CSIZE;
    options.c_cflag |=  CS8;        /* 8 data bits */
    options.c_cflag |=  (CLOCAL | CREAD);

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;

    /* VMIN=1, VTIME=1 -> 阻塞读, 100ms超时 */
    options.c_cc[VMIN]  = 0;
    options.c_cc[VTIME] = 1;

    tcsetattr(fd, TCSANOW, &options);
    tcflush(fd, TCIOFLUSH);

    printf("Listening...\n");

    unsigned char buf[BUF_SIZE];
    int count = 0;

    while (keep_running) {
        int n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            count++;

            printf("\n[#%03d] ", count);
            printf("HEX: ");
            print_hex(buf, n);
            printf("  |  ASCII: \"");
            print_ascii(buf, n);
            printf("\"\n");

            /* 如果数据看起来像卡号 (4或7字节), 特别标注 */
            if (n == 4 || n == 7 || n == 10) {
                printf("       ^^^ Possible card UID (%d bytes)\n", n);
                printf("       Add this to whitelist[] in rfidcard.c:\n");
                printf("       \"");
                print_hex(buf, n);
                printf("\",\n");
            }

            fflush(stdout);
        }
    }

    close(fd);
    printf("\n\nExited. Remember to add your card UID to the whitelist!\n");
    return 0;
}
