SHELL := /bin/bash
.PHONY: run run-bat _run-selected prepare all clean latest fpu $(ALL)

NPC_HOME ?= $(abspath $(CURDIR)/../../../npc)
CONSTRUCTION_MANAGER := $(NPC_HOME)/scripts/construction-manager.sh
PERFORMANCE_SUMMARY := $(NPC_HOME)/scripts/performance-summary.sh
RESULT := .result
COLOR_RED := \033[1;31m
COLOR_GREEN := \033[1;32m
COLOR_NONE := \033[0m

INTEGER_TESTS := $(basename $(notdir $(shell find tests -name '*.c' -print)))
FPU_TEST_ENABLED := $(if $(filter 1,$(FPU_TEST)),1,0)
DEFAULT_ALL := $(INTEGER_TESTS) $(if $(FPU_TEST_ENABLED),fpu)
ALL ?= $(DEFAULT_ALL)
LOG_ROOT ?= $(abspath $(CURDIR)/../../../log/constructions)

comma := ,
VERSION_IDS := $(strip $(subst $(comma), ,$(version)))

# 真正的单 Config 执行只发生在递归目标中；外层 run-bat 可以先拆分多个编号，
# 因而不同 XLEN 的构造不会在同一个 Make 解析期共享 ARCH。
ifeq ($(filter _run-selected,$(MAKECMDGOALS)),_run-selected)
  CONFIG_RESOLUTION := $(strip $(shell $(CONSTRUCTION_MANAGER) resolve "$(NPC_HOME)" "$(strip $(config))" "$(strip $(version))"))
  ifeq ($(CONFIG_RESOLUTION),)
    $(error 无法解析 config/version)
  endif
  CONFIG_WORDS := $(subst |, ,$(CONFIG_RESOLUTION))
  CONFIG_FQCN := $(word 1,$(CONFIG_WORDS))
  CONFIG_SHORT := $(word 2,$(CONFIG_WORDS))
  CONFIG_CAPABILITY := $(word 3,$(CONFIG_WORDS))
  CONFIG_TARGET := $(word 4,$(CONFIG_WORDS))
  CONFIG_XLEN := $(word 5,$(CONFIG_WORDS))
  CONFIG_SCOPE := $(word 6,$(CONFIG_WORDS))
  CONFIG_BOARD := $(word 7,$(CONFIG_WORDS))
  CONFIG_VERSION := $(word 8,$(CONFIG_WORDS))
  CONSTRUCTION_DIR := $(word 9,$(CONFIG_WORDS))
  CONFIG_PROFILE := $(word 10,$(CONFIG_WORDS))
  INFERRED_ARCH := riscv$(CONFIG_XLEN)-nemu
  ifneq ($(strip $(ARCH)),)
    ifneq ($(ARCH),$(INFERRED_ARCH))
      $(error ARCH=$(ARCH) 与 $(CONFIG_FQCN) 固定的 XLEN=$(CONFIG_XLEN) 冲突；应为 $(INFERRED_ARCH))
    endif
  endif
  override ARCH := $(INFERRED_ARCH)
  ifeq ($(CONFIG_CAPABILITY),elaborate-only)
    $(error $(CONFIG_FQCN) 只有 elaborate-only 能力，不能执行 run/run-bat)
  endif
endif

run:
	@test "$(words $(VERSION_IDS))" -le 1 || { echo 'run 只接受一个 version 编号' >&2; exit 2; }
	@set -e; \
		session="$(LOG_ROOT)/runs/$$(date +%Y%m%d%H%M%S)-$$$$"; mkdir -p "$$session"; summary="$$session/summary.tsv"; \
		status=0; \
		$(MAKE) --no-print-directory _run-selected RUN_MODE=interactive \
			config="$(strip $(config))" version="$(strip $(version))" ALL="$(ALL)" \
			build="$(strip $(build))" rebuild="$(strip $(rebuild))" ARCH="$(strip $(ARCH))" \
			RUN_SUMMARY="$$summary" || status=$$?; \
		if test -s "$$summary"; then $(PERFORMANCE_SUMMARY) show "$$summary" | tee "$$session/summary.txt"; fi; \
		exit $$status

run-bat:
	@set -e; \
	session="$(LOG_ROOT)/runs/$$(date +%Y%m%d%H%M%S)-$$$$"; mkdir -p "$$session"; summary="$$session/summary.tsv"; \
	if test -n "$(strip $(version))"; then versions='$(VERSION_IDS)'; else versions=''; fi; \
	status=0; \
	if test -z "$$versions"; then \
		$(MAKE) --no-print-directory _run-selected RUN_MODE=batch config="$(strip $(config))" version= \
			ALL="$(ALL)" build="$(strip $(build))" rebuild="$(strip $(rebuild))" ARCH="$(strip $(ARCH))" \
			RUN_SUMMARY="$$summary" || status=1; \
	else \
		for id in $$versions; do \
			$(MAKE) --no-print-directory _run-selected RUN_MODE=batch config="$(strip $(config))" version="$$id" \
				ALL="$(ALL)" build="$(strip $(build))" rebuild="$(strip $(rebuild))" ARCH="$(strip $(ARCH))" \
				RUN_SUMMARY="$$summary" || status=1; \
		done; \
	fi; \
	if test -s "$$summary"; then $(PERFORMANCE_SUMMARY) show "$$summary" | tee "$$session/summary.txt"; fi; \
	exit $$status

prepare:
	@case "$(build)" in ''|0|1) ;; *) echo 'build 只能是 0 或 1' >&2; exit 2;; esac
	@case "$(rebuild)" in ''|0|1) ;; *) echo 'rebuild 只能是 0 或 1' >&2; exit 2;; esac
	@$(CONSTRUCTION_MANAGER) ensure "$(NPC_HOME)" "$(CONFIG_FQCN)" \
		"$(if $(filter 1,$(build)),1,0)" "$(if $(filter 1,$(rebuild)),1,0)"
	@test -x "$(CONSTRUCTION_DIR)/abi/nemu/nemu-exec" || { echo '构造缺少可执行 NEMU host' >&2; exit 1; }
	@printf '=== Config=%s capability=%s ARCH=%s ===\n' "$(CONFIG_FQCN)" "$(CONFIG_CAPABILITY)" "$(ARCH)"

_run-selected: prepare
	@rm -f "$(RESULT)"; : > "$(RESULT)"
	@$(MAKE) --no-print-directory all RUN_MODE="$(RUN_MODE)" config="$(config)" version="$(version)" \
		ALL="$(ALL)" ARCH="$(ARCH)" CONFIG_FQCN="$(CONFIG_FQCN)" CONFIG_CAPABILITY="$(CONFIG_CAPABILITY)" \
		CONSTRUCTION_DIR="$(CONSTRUCTION_DIR)" CONFIG_PROFILE="$(CONFIG_PROFILE)" RUN_SUMMARY="$(RUN_SUMMARY)"
	@cat "$(RESULT)"
	@! grep -q 'FAIL' "$(RESULT)"

all: $(addprefix Makefile.,$(filter-out fpu,$(ALL))) $(filter fpu,$(ALL))
	@echo "test list [$(words $(ALL)) item(s)]: $(ALL)"

$(filter-out fpu,$(ALL)): %: Makefile.%

Makefile.%: tests/%.c latest
	@printf 'NAME = %s\nSRCS = %s\ninclude $${AM_HOME}/Makefile\n' '$*' '$<' > '$@'
	@set -o pipefail; \
	version_index=$$(sed -n 's/^VERSION_INDEX=//p' "$(CONSTRUCTION_DIR)/construction.env"); \
	log_dir="$(LOG_ROOT)/$$version_index/$*"; mkdir -p "$$log_dir"; \
	goal=run; test "$(RUN_MODE)" = batch && goal=run-bat; \
	if $(MAKE) -s -f '$@' ARCH="$(ARCH)" $$goal \
		NPC_CONSTRUCTION_DIR="$(CONSTRUCTION_DIR)" NPC_CONFIG_FQCN="$(CONFIG_FQCN)" \
		LOG_DIR="$$log_dir" 2>&1 | tee "$$log_dir/output.log"; then \
		result=PASS; printf '[%14s] $(COLOR_GREEN)PASS$(COLOR_NONE)\n' '$*' >> "$(RESULT)"; \
	else \
		result=FAIL; printf '[%14s] $(COLOR_RED)***FAIL***$(COLOR_NONE)\n' '$*' >> "$(RESULT)"; \
	fi; \
	echo "$$result" > "$$log_dir/status"; \
	$(PERFORMANCE_SUMMARY) record "$(CONSTRUCTION_DIR)" '$*' "$$log_dir/output.log" "$$result" "$(RUN_SUMMARY)"
	@rm -f '$@'

fpu: latest
	@set -o pipefail; \
	version_index=$$(sed -n 's/^VERSION_INDEX=//p' "$(CONSTRUCTION_DIR)/construction.env"); \
	log_dir="$(LOG_ROOT)/$$version_index/fpu"; mkdir -p "$$log_dir"; \
	goal=run; test "$(RUN_MODE)" = batch && goal=run-bat; \
	if test "$$(sed -n 's/^F=//p' "$(CONFIG_PROFILE)")" != 1; then \
		printf '%s\n' '当前 Config 未启用 RISC-V F 扩展，不能运行 fpu 用例。' | tee "$$log_dir/output.log"; \
		result=FAIL; \
	elif $(MAKE) -s -C ../fpu-tests ARCH="$(ARCH)" "$$goal" \
		NPC_CONSTRUCTION_DIR="$(CONSTRUCTION_DIR)" NPC_CONFIG_FQCN="$(CONFIG_FQCN)" \
		LOG_DIR="$$log_dir" 2>&1 | tee "$$log_dir/output.log"; then \
		result=PASS; \
	else \
		result=FAIL; \
	fi; \
	echo "$$result" > "$$log_dir/status"; \
	if test "$$result" = PASS; then \
		printf '[%14s] $(COLOR_GREEN)PASS$(COLOR_NONE)\n' fpu >> "$(RESULT)"; \
	else \
		printf '[%14s] $(COLOR_RED)***FAIL***$(COLOR_NONE)\n' fpu >> "$(RESULT)"; \
	fi; \
	$(PERFORMANCE_SUMMARY) record "$(CONSTRUCTION_DIR)" fpu "$$log_dir/output.log" "$$result" "$(RUN_SUMMARY)"

clean:
	rm -rf Makefile.* build/ "$(RESULT)"

latest:
