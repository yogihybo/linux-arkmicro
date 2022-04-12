SRCDIR=./src
PROJECT=libmfc.so
CFLAGS=$(CFLAG)
CFLAGS+=-o2 -fPIC -I./include -DDEC_MODULE_PATH=\"/dev/vdec\" -DMEMALLOC_MODULE_PATH=\"/dev/memalloc\" -DDWL_USE_DEC_IRQ -DDWL_SINGLE_CORE_SW_ONLY
CC=$(CROSS_COMPILE)gcc
SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS=$(patsubst %.c, %.o, $(SRCS))

$(PROJECT):$(OBJS)
	$(CC) $^ -shared -o $@
$(SRCDIR)/%.o:$(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@  

clean:
	rm -f $(SRCDIR)/*.o $(PROJECT)
