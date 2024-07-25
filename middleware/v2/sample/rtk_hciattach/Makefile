CFLAGS := -Wall -g
CC := $(CROSS_COMPILE)gcc
all: rtk_hciattach
OBJS := hciattach.o hciattach_rtk.o hciattach_h4.o rtb_fwc.o

ifeq ($(ARCH), riscv)
OPT_LEVEL += -march=rv64imafdcvxthead -mcmodel=medany -mabi=lp64d
endif

rtk_hciattach: $(OBJS)
	$(CROSS_COMPILE)gcc -o rtk_hciattach $(OBJS) $(OPT_LEVEL)

%.o: %.c
	$(CROSS_COMPILE)gcc -c $< -o $@ $(CFLAGS)

clean:
	rm -f $(OBJS)  rtk_hciattach

tags: FORCE
	ctags -R
	find ./ -name "*.h" -o -name "*.c" -o -name "*.cc" -o -name "*.cpp" > cscope.files
	cscope -bkq -i cscope.files
PHONY += FORCE
FORCE:
