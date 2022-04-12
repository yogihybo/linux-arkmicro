CC=arm-linux-gnueabihf-gcc
#CC=arm-linux-gnueabihf-gcc
#CFLAGS=-Wall -std=c99
TARGET=demo-display
SRCS = main.c carback.c

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