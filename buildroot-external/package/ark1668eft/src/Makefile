PROJECT=ark1668eft
CFLAG=-o2 -fPIC -I$(STAGING_DIR)/usr/include -I$(STAGING_DIR)/usr/include/HAL -DMEMALLOC_MODULE_PATH=\"/dev/memalloc\"
LDFLAG=-L$(STAGING_DIR)/usr/lib -lpthread -lmfc -lasound
CC=$(CROSS_COMPILE)gcc
SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c, %.o, $(SRCS))

$(PROJECT):$(OBJS)
	$(CC) $(LDFLAG) $^ -o $@
%.o:%.c
	$(CC) $(CFLAG) -c $< -o $@

clean:
	rm -f *.o $(PROJECT)
