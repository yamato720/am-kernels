.PHONY: all run gdb clean latest $(ALL) native qemu run-bat run-npc run-npc-bat run-nemu run-nemu-bat gdb-nemu

RESULT = .result
$(shell > $(RESULT))

COLOR_RED   = \033[1;31m
COLOR_GREEN = \033[1;32m
COLOR_NONE  = \033[0m

# RISC-V toolchain
RISCV_PREFIX ?= riscv64-linux-gnu-
RISCV_CC = $(RISCV_PREFIX)gcc
RISCV_CFLAGS = -O2 -static

ALL = $(basename $(notdir $(shell find tests/. -name "*.c")))

all: $(addprefix Makefile., $(ALL))
	@echo "test list [$(words $(ALL)) item(s)]:" $(ALL)

# Native build target for running on host machine
native:
	@echo "Building and running native version..."
	@for test in $(ALL); do \
		echo "Building $$test..."; \
		/bin/echo -e "NAME = $$test\nSRCS = tests/$$test.c\ninclude \$${AM_HOME}/Makefile" > Makefile.$$test.native; \
		if make -s -f Makefile.$$test.native ARCH=native && ./build/$$test-native; then \
			printf "[%14s] $(COLOR_GREEN)PASS$(COLOR_NONE)\n" $$test; \
		else \
			printf "[%14s] $(COLOR_RED)***FAIL***$(COLOR_NONE)\n" $$test; \
		fi; \
		rm -f Makefile.$$test.native; \
	done

# RISC-V Linux user mode build target for QEMU
qemu:
	@echo "Building and running RISC-V Linux user mode with QEMU..."
	@mkdir -p build/qemu
	@for test in $(ALL); do \
		if ./compile-qemu.sh $$test; then \
			if qemu-riscv64 build/qemu/$$test-riscv64-linux 2>&1; then \
				printf "[%14s] $(COLOR_GREEN)PASS$(COLOR_NONE)\n" $$test; \
			else \
				printf "[%14s] $(COLOR_RED)***FAIL***$(COLOR_NONE)\n" $$test; \
			fi; \
		else \
			printf "[%14s] $(COLOR_RED)***SKIP***$(COLOR_NONE)\n" $$test; \
		fi; \
	done

$(ALL): %: Makefile.%  # ALL 依赖于Makefile.ALL

Makefile.%: tests/%.c latest
	@/bin/echo -e "NAME = $*\nSRCS = $<\ninclude $${AM_HOME}/Makefile" > $@
	@if make -s -f $@ ARCH=$(ARCH) $(MAKECMDGOALS); then \
		printf "[%14s] $(COLOR_GREEN)PASS$(COLOR_NONE)\n" $* >> $(RESULT); \
	else \
		echo make -s -f $@ ARCH=$(ARCH) $(MAKECMDGOALS); \
		printf "[%14s] $(COLOR_RED)***FAIL***$(COLOR_NONE)\n" $* >> $(RESULT); \
	fi
	-@rm -f Makefile.$*
# $@ - 目标文件名
# $< - 第一个依赖文件
# $* - 模式匹配中 % 匹配的部分
# 在@if make -s -f $@ ARCH=$(ARCH) $(MAKECMDGOALS)下执行，即make -s -f Makefile.string ARCH=riscv64-nemu run
run: all
	@cat $(RESULT)
	@rm $(RESULT)

run-bat: all
	@cat $(RESULT)
	@rm $(RESULT)

run-npc: all
	@cat $(RESULT)
	@rm $(RESULT)

run-npc-bat: all
	@cat $(RESULT)
	@rm $(RESULT)

run-nemu: all
	@cat $(RESULT)
	@rm $(RESULT)

run-nemu-bat: all
	@cat $(RESULT)
	@rm $(RESULT)

gdb-nemu: all
	@cat $(RESULT)
	@rm $(RESULT)

run-npc-update:
	$(MAKE) -C $(NPC_HOME) chisel-cpu-lib TRACE_NEMU=1
	rm -f $(NEMU_HOME)/build/riscv64-nemu-interpreter
	$(MAKE) -C $(NEMU_HOME) ISA=riscv64 USENPC=1

run-npc-update-trace:
	$(MAKE) -C $(NPC_HOME) chisel-cpu-lib TRACE_NEMU=1

gdb: all

gdb-npc: all

clean:
	rm -rf Makefile.* build/

latest:
