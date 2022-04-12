CC=/home/jepson/work-xh/test/linux-arkmicro-v4l2/buildroot-external/toolchain/gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
#CC=arm-linux-gnueabihf-gcc
#CFLAGS=-Wall -std=c99
TARGET=demo-v4l2
SRCS = main.c

INC = -I./include -I./include/sys
LDFLAGS += -L./lib
DLIBS = -lGAL -lmfc -larkapi -lpthread  -lrt -lGAL
OBJS = $(SRCS:.c=.o)

#CFLAGS += -DgcdENABLE_3D=0
CFLAGS += $(INC)

$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(DLIBS)

clean:
	rm -rf $(TARGET) $(OBJS)

%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $<
