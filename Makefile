.PHONY := kbuild all clean stat include_build unittest

srctree := $(shell pwd)
include tools/Kbuild.include

# Check if V options is set by user, if V=1, debug mode is set
# e.g. make V=1 produces all the commands being executed through
# the building process
ifeq ("$(origin V)", "command line")
	KBUILD_VERBOSE = $(V)
endif
ifndef KBUILD_VERBOSE
	KBUILD_VERBOSE = 0
endif
ifeq ($(KBUILD_VERBOSE),1)
	Q =
else
	MAKEFLAGS += --no-print-directory -s
	Q = @
endif

export NO_MAKE_DEPEND := $(N)
export KBUILD_VERBOSE
export Q
export srctree \\
export includes := $(shell find include -name "*.h")
export TEXT_OFFSET := 2048
export SREC_INCLUDE := include/srec
export INCLUDE_PATH := -I./include/posix_include -I./include
export CURR_UNIX_TIME := $(shell date +%s)
export WINIX_CFLAGS := -D__wramp__
export COMMON_CFLAGS := -DTEXT_OFFSET=$(TEXT_OFFSET) -D_DEBUG 
export CFLAGS := $(WINIX_CFLAGS) $(COMMON_CFLAGS)
export GCC_FLAG := -Wimplicit-fallthrough -Wsequence-point -Wswitch-default -Wswitch-unreachable \
		-Wswitch-enum -Wstringop-truncation -Wbool-compare -Wtautological-compare -Wfloat-equal \
		-Wshadow=global -Wpointer-arith -Wpointer-compare -Wcast-align -Wwrite-strings \
		-Wdangling-else -Wlogical-op -Wunused -Wpointer-to-int-cast -Wno-discarded-qualifiers \
		-Wno-builtin-declaration-mismatch -pedantic -Werror

# List of user libraries used by the kernel
KLIB_O = lib/syscall/wramp_syscall.o lib/ipc/ipc.o \
		lib/ansi/string.o lib/util/util.o lib/gen/ucontext.o lib/stdlib/atoi.o\
		lib/syscall/debug.o lib/posix/_sigset.o lib/ansi/rand.o
L_HEAD = winix/limits/limits_head.o
L_TAIL = winix/limits/limits_tail.o
KERNEL_O = winix/*.o kernel/system/*.o kernel/*.o fs/*.o fs/system/*.o driver/*.o include/*.o
ALLDIR = winix lib init user kernel fs driver
ALLDIR_CLEAN = winix lib init user kernel fs driver include
FS_DEPEND = fs/*.c fs/system/*.c fs/fsutil/*.c winix/bitmap.c
DISK = include/disk.c
START_TIME_FILE = include/startup_time.c
SREC = $(shell find $(SREC_INCLUDE) -name "*.srec")

all:| fsutil kbuild $(DISK) include_build
	$(Q)wlink $(LDFLAGS) -Ttext 1024 -v -o winix.srec \
	$(L_HEAD) $(KERNEL_O) $(KLIB_O) $(L_TAIL) > $(SREC_INCLUDE)/winix.verbose
ifeq ($(KBUILD_VERBOSE),0)
	@echo "LD \t winix.srec"
endif

unittest:
	$(Q)./fsutil -d -t $(TEXT_OFFSET)

fsutil: $(FS_DEPEND)
	$(Q)gcc -g -DFSUTIL $(GCC_FLAG) $(COMMON_CFLAGS) -I./include/fs_include -I./include $^ -o fsutil
ifeq ($(KBUILD_VERBOSE),0)
	@echo "CC \t fsutil"
endif

kbuild: $(ALLDIR)
$(ALLDIR): FORCE
	$(Q)$(MAKE) $(build)=$@

$(DISK): $(SREC) fsutil
	$(Q)./fsutil -t $(TEXT_OFFSET) -o $(DISK) -s $(SREC_INCLUDE) -u $(CURR_UNIX_TIME)
ifeq ($(KBUILD_VERBOSE),0)
	@echo "LD \t disk.c"
endif
	
include_build:
	$(Q)echo "unsigned int start_unix_time=$(CURR_UNIX_TIME);\n" > $(START_TIME_FILE)
	$(Q)$(MAKE) $(build)=include

clean:
	$(Q)rm -f fsutil
	$(Q)$(MAKE) $(cleanall)='$(ALLDIR_CLEAN)'

stat:
	@echo "C Lines: "
	@find . -type d -name "include" -prune -o -name "*.c"  -exec cat {} \; | wc -l
	@echo "Header LoC: "
	@find . -name "*.h" -exec cat {} \; | wc -l
	@echo "Assembly LoC: "
	@find . -name "*.s" -exec cat {} \; | wc -l

FORCE:

# DO NOT DELETE
