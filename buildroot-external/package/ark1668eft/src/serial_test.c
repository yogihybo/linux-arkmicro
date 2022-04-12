#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>

#include "ftcfg.h"
#include "ftypes.h"
#include "utils.h"
#include "ark1668eft.h"

#define RECEIVE_LEN		128
#define UART_START		0
#define UART_NUM		6

struct uart_priv {
    int fd;
    pthread_t tid;
    char *revbuf;
    int finish;
}uartport[UART_NUM];

static int open_port(int comport)
{
    char dev[16];
    long vdisable;
    int fd = -1, ret = -1;

    if (comport > 5)
    	return -1;

	if (comport < 4) {
		sprintf(dev, "/dev/ttyS%d", comport);
	} else if (comport < 6) {
		sprintf(dev, "/dev/ttyHS%d", comport - 4);
	}

    fd = open(dev, O_RDWR | O_NOCTTY);//|O_NOCTTY|O_NDELAY
    if (fd < 0) {
        printf("Can't Open Serial Port ttyS%d", comport);
        return -1;
    }

	if (fcntl(fd, F_SETFL, 0) < 0) {
		printf("fcntl failed!\n");
		goto exit;
	}
    ret = 0;
exit:
	if (ret < 0)
		close(fd);
	return fd;
}

static int set_opt(int fd, int nSpeed, int nBits, char nEvent, int nStop)
{
	struct termios newtio;

	bzero(&newtio, sizeof(newtio));
	if (tcgetattr(fd, &newtio) != 0) {
		perror("SetupSerial 1");
		return -1;
	}

	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;

	switch(nBits) {
	case 7:
		newtio.c_cflag |= CS7;
		break;
	case 8:
		newtio.c_cflag |= CS8;
		break;
	}

	switch(nEvent) {
	case 'O':
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
		break;
	case 'E':
		newtio.c_iflag |= (INPCK | ISTRIP);
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
		break;
	case 'N':
		newtio.c_cflag &= ~PARENB;
		break;
	}

	switch(nSpeed) {
	case 2400:
		cfsetispeed(&newtio, B2400);
		cfsetospeed(&newtio, B2400);
		break;

	case 4800:
		cfsetispeed(&newtio, B4800);
		cfsetospeed(&newtio, B4800);
		break;

	case 9600:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;

	case 19200:
		cfsetispeed(&newtio, B19200);
		cfsetospeed(&newtio, B19200);
		break;

	case 115200:
		cfsetispeed(&newtio, B115200);
		cfsetospeed(&newtio, B115200);
		break;

	case 460800:
		cfsetispeed(&newtio, B460800);
		cfsetospeed(&newtio, B460800);
		break;

	default:
		cfsetispeed(&newtio, B9600);
		cfsetospeed(&newtio, B9600);
		break;
	}

	if (nStop == 1)
		newtio.c_cflag &= ~CSTOPB;
	else if (nStop == 2)
		newtio.c_cflag |= CSTOPB;

	newtio.c_lflag &= ~(ICANON | ISIG | ECHO | IEXTEN);
	newtio.c_iflag &= ~(INPCK|BRKINT|ICRNL|ISTRIP|IXON);
	newtio.c_oflag  &= ~OPOST;

	newtio.c_cc[VTIME] = 1;
	newtio.c_cc[VMIN] = 255;

	tcflush(fd,TCIFLUSH);

	if ((tcsetattr(fd, TCSANOW, &newtio)) != 0) {
		perror("com set error");
		return -1;
	}

	return 0;
}

void *uart_receive_thread(void *arg)
{
    struct uart_priv *port = (struct uart_priv *)arg;

    read(port->fd, port->revbuf, RECEIVE_LEN);
    port->finish = 1;

    return 0;
}

void *serial_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;
    int fddata;
    int i, j;
    char buf[RECEIVE_LEN];
    int timeout = TEST_TIMEOUT / 2 / 100;
	int errcount[UART_NUM] = {0};
	int errexit = 0;

    for (i = 0; i < UART_NUM; i++) {
        uartport[i].fd = -1;
        uartport[i].finish = 0;
        uartport[i].revbuf = malloc(RECEIVE_LEN);
        if (!uartport[i].revbuf) {
            printf("uart malloc fail.\n");
            goto err;
        }
    }

    for (i = UART_START; i < UART_NUM; i++) {
        uartport[i].fd = open_port(i);
        if (set_opt(uartport[i].fd, 115200, 8, 'N', 1) < 0) {
            perror("set_opt error");
            goto err;
        }
		pthread_create(&uartport[i].tid, NULL,
			uart_receive_thread, &uartport[i]);
		pthread_detach(uartport[i].tid);
    }

    usleep(100000);

    fddata = open(SERIAL_DATA_PATH, O_RDONLY);
    if (fddata < 0) {
        printf("open serial data fail.\n");
        goto err;
    }
    read(fddata, buf, RECEIVE_LEN);
    for (i = UART_START; i < UART_NUM; i++) {
        write(uartport[i].fd, buf, RECEIVE_LEN);
    }

	do {
		for (i = UART_START; i < UART_NUM; i++) {
			if (!uartport[i].finish)
				break;
		}

		if (i == UART_NUM)
			break;

		usleep(100000);
	} while(timeout--);

    for (i = UART_START; i < UART_NUM; i++) {
		for (j = 0; j < RECEIVE_LEN - 1; j++) {
			if (uartport[i].revbuf[j] != buf[j] &&
				/* lost a byte sometimes */
				uartport[i].revbuf[j] != buf[j+1]) {
				printf("uart[%d] data diff 0x%.2x, 0x%.2x.\n", i, uartport[i].revbuf[j], buf[j]);
				errcount[i]++;
			}
		}
    }

	for (i = UART_START; i < UART_NUM; i++) {
		if (errcount[i] > 5) {
			printf("uart %d errcount=%d.\n", i, errcount[i]);
			errexit = 1;
		}
	}

	if (errexit)
		goto err;

    for (i = UART_START; i < UART_NUM; i++) {
        close(uartport[i].fd);
        free(uartport[i].revbuf);
    }

    rt->finish = 1;
    rt->pass = 1;

    /* gpio_export(81);
    gpio_set_dir(81, "out");
    gpio_set_value(81, 1); */

	return (void*)0;

err:
    /* gpio_export(81);
    gpio_set_dir(81, "out");
    gpio_set_value(81, 0); */

    for (i = 0; i < UART_NUM; i++) {
        if (uartport[i].fd > 0)
            close(uartport[i].fd);
        if (uartport[i].revbuf)
            free(uartport[i].revbuf);
    }
    rt->finish = 1;
    return (void*)-1;
}