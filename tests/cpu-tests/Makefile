SHELL := /bin/bash
.PHONY: config-list version run run-bat _prepare-selected _run-selected _check-run-mode _run-one clean latest fpu $(ALL)

NPC_HOME ?= $(abspath $(CURDIR)/../../../npc)
CONSTRUCTION_MANAGER := $(NPC_HOME)/scripts/construction-manager.sh
PERFORMANCE_SUMMARY := $(NPC_HOME)/scripts/performance-summary.sh
COLOR_RED := \033[1;31m
COLOR_GREEN := \033[1;32m
COLOR_NONE := \033[0m

FPGA_FALLBACK_TEST := fpga-fallback
INTEGER_TESTS := $(filter-out $(FPGA_FALLBACK_TEST),$(basename $(notdir $(shell find tests -name '*.c' -print))))
FPU_TEST_ENABLED := $(if $(filter 1,$(FPU_TEST)),1,0)
DEFAULT_ALL := $(INTEGER_TESTS) $(if $(FPU_TEST_ENABLED),fpu $(FPGA_FALLBACK_TEST))
ALL ?= $(DEFAULT_ALL)
LOG_ROOT ?= $(abspath $(CURDIR)/../../../log/constructions)

ifneq ($(origin host_rebuild),undefined)
  $(error host_rebuild 已删除，请使用 host-rebuild=1)
endif

ifneq ($(strip $(rebuild)),)
  $(error rebuild 参数已删除，请先执行 make -C $(NPC_HOME) rebuild config=<Config>)
endif

comma := ,
VERSION_IDS := $(strip $(subst $(comma), ,$(version)))
RUN_JOBS := $(if $(strip $(jobs)),$(strip $(jobs)),1)
RUN_RESET := $(if $(strip $(reset)),$(strip $(reset)),0)
RESET_ARGUMENT := $(if $(strip $(reset)),reset="$(strip $(reset))",)

ifneq ($(filter-out 0 1,$(RUN_RESET)),)
  $(error reset 只能是 0 或 1)
endif

config-list:
	@$(MAKE) -C "$(NPC_HOME)" config-list

version:
	@$(MAKE) -C "$(NPC_HOME)" version config="$(strip $(config))" version="$(strip $(version))" \
		D="$(strip $(D))" delete="$(strip $(delete))" yes="$(strip $(yes))"

# 每个递归任务只解析一个硬件 Config/版本，因此不同 XLEN 的任务不会共享 Make 的
# ARCH 解析期状态。`_run-one` 由 run-bat 的父进程传入独立的 RUN_RESULT_DIR。
ifneq ($(filter _prepare-selected _run-selected _run-one,$(MAKECMDGOALS)),)
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
  ifeq ($(filter run batch,$(CONFIG_CAPABILITY)),)
    $(error $(CONFIG_FQCN) 不是可运行 Config，不能执行 run/run-bat)
  endif
  ifneq ($(strip $(reset)),)
    ifneq ($(CONFIG_SCOPE),fpga)
      $(error reset=1 仅适用于 FPGA Config)
    endif
  endif
endif

run:
	@test "$(words $(VERSION_IDS))" -le 1 || { echo 'run 只接受一个 version 编号' >&2; exit 2; }
	@set -e; \
		session="$(LOG_ROOT)/runs/$$(date +%Y%m%d%H%M%S)-$$$$"; mkdir -p "$$session/items"; summary="$$session/summary.tsv"; \
		status=0; \
		$(MAKE) --no-print-directory _run-selected RUN_MODE=batch \
			config="$(strip $(config))" version="$(strip $(version))" ALL="$(ALL)" \
			build="$(strip $(build))" host-rebuild="$(strip $(host-rebuild))" \
			$(RESET_ARGUMENT) \
			ARCH="$(strip $(ARCH))" RUN_RESULT_ROOT="$$session/items" RUN_SUMMARY="$$summary" || status=$$?; \
		if test -s "$$summary"; then $(PERFORMANCE_SUMMARY) show "$$summary" | tee "$$session/summary.txt"; fi; \
		if find "$$session/items" -name summary.env -type f -print -quit | grep -q .; then \
			printf '=== 单项详细报告 ===\n'; $(PERFORMANCE_SUMMARY) details "$$session/items" | tee "$$session/details.txt"; \
		fi; \
		rm -rf "$$session/items"; \
		exit $$status

run-bat:
	@case "$(RUN_JOBS)" in -1|[1-9]|[1-9][0-9]*) ;; *) echo 'jobs 只能是正整数或 -1' >&2; exit 2;; esac
	@case "$(build)" in ''|0|1) ;; *) echo 'build 只能是 0 或 1' >&2; exit 2;; esac
	@case "$(host-rebuild)" in ''|0|1) ;; *) echo 'host-rebuild 只能是 0 或 1' >&2; exit 2;; esac
	@set -e; \
		session="$(LOG_ROOT)/runs/$$(date +%Y%m%d%H%M%S)-$$$$"; mkdir -p "$$session/items"; summary="$$session/summary.tsv"; completion="$$session/completion.tsv"; counter="$$session/completion.counter"; printf '0\n' > "$$counter"; \
		active_pids=(); \
		on_interrupt() { \
			trap - INT TERM; \
			printf '批次运行被中断；等待已启动任务退出，现场保留在：%s\n' "$$session" >&2; \
			printf 'INTERRUPTED\n' > "$$session/status"; \
			for pid in "$${active_pids[@]}"; do \
				if kill -0 "$$pid" 2>/dev/null; then kill -INT "$$pid" 2>/dev/null || true; fi; \
			done; \
			for pid in "$${active_pids[@]}"; do wait "$$pid" 2>/dev/null || true; done; \
			exit 130; \
		}; \
		trap on_interrupt INT TERM; \
		if test -n "$(strip $(version))"; then selectors='$(VERSION_IDS)'; else selectors='@config'; fi; \
		effective_jobs="$(RUN_JOBS)"; fpga_jobs_notice=0; \
		for selector in $$selectors; do \
			if test "$$selector" = @config; then selected_config='$(strip $(config))'; selected_version=''; else selected_config='$(strip $(config))'; selected_version="$$selector"; fi; \
			resolved="$$($(CONSTRUCTION_MANAGER) resolve "$(NPC_HOME)" "$$selected_config" "$$selected_version")"; \
			selected_scope="$$(printf '%s\n' "$$resolved" | cut -d'|' -f6)"; \
			if test "$$selected_scope" = fpga; then \
				if test "$$effective_jobs" != 1 && test "$$fpga_jobs_notice" = 0; then \
					printf 'FPGA 不支持 jobs=%s；按 jobs=1 串行运行。\n' "$$effective_jobs" >&2; fpga_jobs_notice=1; \
				fi; \
				effective_jobs=1; \
			fi; \
			$(MAKE) --no-print-directory _prepare-selected config="$$selected_config" version="$$selected_version" \
				build="$(strip $(build))" host-rebuild="$(strip $(host-rebuild))" $(RESET_ARGUMENT) ARCH="$(strip $(ARCH))"; \
		done; \
		total=$$(( $$(wc -w <<< "$$selectors") * $(words $(ALL)) )); active=0; status=0; \
		report_progress() { \
			completed=$$(find "$$session/items" -name completed -type f -print | wc -l); \
			if test -t 1; then \
				width=24; filled=$$(( completed * width / total )); empty=$$(( width - filled )); \
				printf -v bar '%*s' "$$filled" ''; bar=$${bar// /#}; printf -v rest '%*s' "$$empty" ''; rest=$${rest// /-}; \
				printf '\r\033[2K批次 [%d/%d] [%s%s]' "$$completed" "$$total" "$$bar" "$$rest"; \
			else \
				printf '批次进度：%s/%s\n' "$$completed" "$$total"; \
			fi; \
		}; \
		printf '批次：%s 个任务，最大并行度 %s\n' "$$total" "$$effective_jobs"; \
		for selector in $$selectors; do \
			if test "$$selector" = @config; then selected_config='$(strip $(config))'; selected_version=''; selector_key='config-$(strip $(config))'; else selected_config='$(strip $(config))'; selected_version="$$selector"; selector_key="version-$$selector"; fi; \
			for test_name in $(ALL); do \
				item="$$session/items/$$selector_key/$$test_name"; \
				mkdir -p "$$item"; \
				label="$${selected_version:-$(strip $(config))}"; \
				printf '启动：[%s] %s\n' "$$label" "$$test_name"; \
				( exec $(MAKE) --no-print-directory _run-one RUN_MODE=batch TEST="$$test_name" \
					config="$$selected_config" version="$$selected_version" ARCH="$(strip $(ARCH))" \
					$(RESET_ARGUMENT) \
					RUN_RESULT_DIR="$$item" RUN_COMPLETION_COUNTER="$$counter" > "$$item/runner.log" 2>&1 ) & \
				active_pids+=("$$!"); \
				active=$$((active + 1)); \
				if test "$$effective_jobs" != -1 && test "$$active" -ge "$$effective_jobs"; then \
					if ! wait -n; then status=1; fi; active=$$((active - 1)); report_progress; \
				fi; \
			done; \
		done; \
		while test "$$active" -gt 0; do if ! wait -n; then status=1; fi; active=$$((active - 1)); report_progress; done; \
		if test -t 1; then printf '\n'; fi; \
		$(PERFORMANCE_SUMMARY) collect "$$session/items" "$$completion" completion; \
		$(PERFORMANCE_SUMMARY) collect "$$session/items" "$$summary" stable; \
		if test $$(wc -l < "$$completion") -gt 1; then printf '=== 按完成顺序的单项性能 ===\n'; $(PERFORMANCE_SUMMARY) show "$$completion" | tee "$$session/completion.txt"; fi; \
		if test $$(wc -l < "$$summary") -gt 1; then printf '=== 稳定排序的性能对比 ===\n'; $(PERFORMANCE_SUMMARY) show "$$summary" | tee "$$session/summary.txt"; fi; \
		if find "$$session/items" -name summary.env -type f -print -quit | grep -q .; then \
			printf '=== 单项详细报告 ===\n'; $(PERFORMANCE_SUMMARY) details "$$session/items" | tee "$$session/details.txt"; \
		fi; \
		while IFS= read -r detail; do \
			if test "$$(sed -n 's/^RESULT=//p' "$$detail")" = FAIL; then \
				item=$$(dirname $$(dirname "$$detail")); printf '=== 失败项：%s ===\n' "$$item"; \
				tail -n 40 "$$item/log/output.log" 2>/dev/null || tail -n 40 "$$item/runner.log" 2>/dev/null || true; \
			fi; \
		done < <(find "$$session/items" -name summary.env -type f -print | LC_ALL=C sort); \
		trap - INT TERM; \
		rm -rf "$$session/items" "$$counter" "$$counter.lock"; \
		exit $$status

prepare:
	@case "$(build)" in ''|0|1) ;; *) echo 'build 只能是 0 或 1' >&2; exit 2;; esac
	@case "$(host-rebuild)" in ''|0|1) ;; *) echo 'host-rebuild 只能是 0 或 1' >&2; exit 2;; esac
	@$(CONSTRUCTION_MANAGER) ensure "$(NPC_HOME)" "$(CONFIG_FQCN)" \
		"$(if $(filter 1,$(build)),1,0)" "$(if $(filter 1,$(host-rebuild)),1,0)"
	@test -x "$(CONSTRUCTION_DIR)/abi/nemu/nemu-exec" || { echo '构造缺少可执行 NEMU host' >&2; exit 1; }
	@printf '=== Config=%s ARCH=%s runtime=%s ===\n' "$(CONFIG_FQCN)" "$(ARCH)" "$(shell sed -n 's/^NEMU_BACKEND=//p' "$(CONFIG_PROFILE)")"

_prepare-selected: prepare

_check-run-mode:
	@test "$(RUN_MODE)" != interactive || test "$(CONFIG_CAPABILITY)" != batch || \
		{ echo '该 PerformanceMonitor Config 仅支持 run-bat。' >&2; exit 2; }

_run-selected: _check-run-mode prepare
	@set -e; \
		root="$(RUN_RESULT_ROOT)"; mkdir -p "$$root"; status=0; \
		for test_name in $(ALL); do \
			$(MAKE) --no-print-directory _run-one RUN_MODE="$(RUN_MODE)" TEST="$$test_name" \
				config="$(config)" version="$(version)" ARCH="$(ARCH)" $(RESET_ARGUMENT) RUN_RESULT_DIR="$$root/$$test_name" || status=1; \
		done; \
		$(PERFORMANCE_SUMMARY) collect "$$root" "$(RUN_SUMMARY)" stable; \
		exit $$status

_run-one:
	@test -n "$(TEST)" || { echo '_run-one 必须提供 TEST=<用例>' >&2; exit 2; }
	@test -n "$(RUN_RESULT_DIR)" || { echo '_run-one 必须提供 RUN_RESULT_DIR=<隔离目录>' >&2; exit 2; }
	@test -x "$(CONSTRUCTION_DIR)/abi/nemu/nemu-exec" || { echo '预检后的构造缺少可执行 NEMU host' >&2; exit 1; }
	@set -o pipefail; \
		item="$(RUN_RESULT_DIR)"; work="$$item/work"; log_dir="$$item/log"; mkdir -p "$$work" "$$log_dir"; \
		case "$(RUN_MODE)" in interactive) goal=run ;; batch) goal=run-bat ;; *) echo 'RUN_MODE 必须是 interactive 或 batch' >&2; exit 2 ;; esac; result=FAIL; \
		if test "$(TEST)" = fpu; then \
			if test "$$(sed -n 's/^F=//p' "$(CONFIG_PROFILE)")" != 1; then \
				printf '%s\n' '当前 Config 未启用 RISC-V F 扩展，不能运行 fpu 用例。' | tee "$$log_dir/output.log"; \
				elif $(MAKE) -s -C ../fpu-tests ARCH="$(ARCH)" "$$goal" \
					NPC_CONSTRUCTION_DIR="$(CONSTRUCTION_DIR)" NPC_CONFIG_FQCN="$(CONFIG_FQCN)" \
					$(RESET_ARGUMENT) \
				NEMU_RUNTIME_LABEL="$(TEST)" \
					LOG_DIR="$$log_dir" AM_BUILD_ROOT="$$work/am-build" 2>&1 | tee "$$log_dir/output.log"; then result=PASS; fi; \
		else \
			test -f "tests/$(TEST).c" || { echo "未知 CPU 用例：$(TEST)" >&2; exit 2; }; \
			printf 'NAME = %s\nSRCS = %s\ninclude $${AM_HOME}/Makefile\n' "$(TEST)" "$(abspath tests/$(TEST).c)" > "$$work/Makefile"; \
					if $(MAKE) -s -f "$$work/Makefile" ARCH="$(ARCH)" "$$goal" \
						NPC_CONSTRUCTION_DIR="$(CONSTRUCTION_DIR)" NPC_CONFIG_FQCN="$(CONFIG_FQCN)" \
						$(RESET_ARGUMENT) \
					NEMU_RUNTIME_LABEL="$(TEST)" \
					LOG_DIR="$$log_dir" AM_BUILD_ROOT="$$work/am-build" 2>&1 | tee "$$log_dir/output.log"; then result=PASS; fi; \
		fi; \
		finished_ns=$$(date +%s%N); \
		echo "$$result" > "$$log_dir/status"; \
		if test "$$result" = PASS; then printf '[%14s] $(COLOR_GREEN)PASS$(COLOR_NONE)\n' "$(TEST)"; else printf '[%14s] $(COLOR_RED)***FAIL***$(COLOR_NONE)\n' "$(TEST)"; fi; \
		if test -n "$(RUN_COMPLETION_COUNTER)"; then \
			$(PERFORMANCE_SUMMARY) record "$(CONSTRUCTION_DIR)" "$(TEST)" "$$log_dir/output.log" "$$result" "$$item/summary.tsv" "$$finished_ns" "$(RUN_COMPLETION_COUNTER)"; \
			printf '%s\n' "$$finished_ns" > "$$item/completed"; \
		else \
			$(PERFORMANCE_SUMMARY) record "$(CONSTRUCTION_DIR)" "$(TEST)" "$$log_dir/output.log" "$$result" "$$item/summary.tsv"; \
		fi; \
		test "$$result" = PASS

clean:
	rm -rf Makefile.* build/

latest:
