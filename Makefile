# File: /home/kami/ics-guard/Makefile
# ==============================================================================
# ARYORITHM TECHNOLOGIES B.V. — CRITICAL INFRASTRUCTURE & OT SYSTEMS
# Project: ics-guard (Industrial SCADA, Substation & Refinery Defense Engine)
# Standards: IEC 62443-4-2, IEC 60870-5-104, Modbus TCP, S7Comm, IEEE C37.118
# Invariant: Sub-120 ns Deterministic Tier-1 Gate | Zero Dynamic Alloc
# ==============================================================================

SHELL := /bin/bash
.DEFAULT_GOAL := help

PROJECT_DIR   := $(shell pwd)
BUILD_DIR     := $(PROJECT_DIR)/build
SCENARIOS_DIR := $(PROJECT_DIR)/scenarios/production_master_grid
MATRIX_DIR    := /home/kami/sentinel-matrix
NEXUS_DIR     := /home/kami/sentinel-nexus
ETC_HARNESSES := /etc/sentinel/lab/harnesses
OPT_HARNESSES := /opt/sentinel-stack/harnesses
JOBS          ?= $(shell nproc)

CYAN   := \033[1;36m
GREEN  := \033[1;32m
YELLOW := \033[1;33m
RED    := \033[1;31m
WHITE  := \033[1;37m
RESET  := \033[0m

.PHONY: help
help:
	@echo -e "$(CYAN)================================================================================$(RESET)"
	@echo -e "$(WHITE)  ICS-GUARD: INDUSTRIAL SCADA & CRITICAL INFRASTRUCTURE DEFENSE ENGINE$(RESET)"
	@echo -e "$(CYAN)================================================================================$(RESET)"
	@echo -e "Deterministic sub-120 ns validation engine securing substations, PLCs & reactors."
	@echo -e ""
	@echo -e "$(YELLOW)System Initialization:$(RESET)"
	@echo -e "  $(GREEN)make init$(RESET)         Hard copy all ICS research harnesses to /etc and /opt"
	@echo -e ""
	@echo -e "$(YELLOW)Core Build & Test Commands:$(RESET)"
	@echo -e "  $(GREEN)make build$(RESET)        Compile release targets (-O3 -march=native)"
	@echo -e "  $(GREEN)make debug$(RESET)        Compile debug targets with Address/UB sanitizers"
	@echo -e "  $(GREEN)make test$(RESET)         Run all unit, ABI, and Tier-1/Tier-2 pipeline tests"
	@echo -e "  $(GREEN)make bench$(RESET)        Execute hardware latency benchmark (< 120 ns SLA)"
	@echo -e "  $(GREEN)make install$(RESET)      Install libraries, CLI, mock_plc & plugin system-wide"
	@echo -e ""
	@echo -e "$(YELLOW)Ecosystem Synchronization:$(RESET)"
	@echo -e "  $(GREEN)make sync-matrix$(RESET)  Deploy libics_guard.so & mock_plc to sentinel-matrix"
	@echo -e "  $(GREEN)make sync-web$(RESET)     Sync base web dashboard from sentinel-nexus to matrix"
	@echo -e "  $(GREEN)make sync-all$(RESET)     Full synchronization (install + sync-matrix + sync-web)"
	@echo -e ""
	@echo -e "$(YELLOW)Digital Twin Simulation Mesh:$(RESET)"
	@echo -e "  $(GREEN)make grid-up$(RESET)      Deploy production_master_grid simulation mesh"
	@echo -e "  $(GREEN)make grid-down$(RESET)    Teardown production_master_grid and release subnets"
	@echo -e "  $(GREEN)make grid-status$(RESET)  Display running grid containers and ports"
	@echo -e "  $(GREEN)make grid-logs$(RESET)    Stream live logs from Edge Substation 01"
	@echo -e ""
	@echo -e "$(YELLOW)Clean & Housekeeping:$(RESET)"
	@echo -e "  $(GREEN)make clean$(RESET)        Remove build artifacts"
	@echo -e "  $(GREEN)make distclean$(RESET)    Full wipe of build, caches, and dangling networks"
	@echo -e "$(CYAN)================================================================================$(RESET)"

.PHONY: init
init:
	@echo -e "$(CYAN)================================================================================$(RESET)"
	@echo -e "$(WHITE)  INITIALIZING PERMANENT ICS HARNESSES (HARD COPY - NO SYMLINKS)               $(RESET)"
	@echo -e "$(CYAN)================================================================================$(RESET)"
	@sudo mkdir -p $(ETC_HARNESSES) $(OPT_HARNESSES)
	@chmod +x harnesses/*.py 2>/dev/null || true
	@for f in harnesses/*.py; do \
		if [ -f "$$f" ]; then \
			base=$$(basename $$f); \
			hyphen_base=$$(echo $$base | tr '_' '-'); \
			sudo rm -f $(ETC_HARNESSES)/$$base $(ETC_HARNESSES)/$$hyphen_base; \
			sudo cp -f $$f $(ETC_HARNESSES)/$$base; \
			sudo cp -f $$f $(ETC_HARNESSES)/$$hyphen_base; \
			sudo rm -f $(OPT_HARNESSES)/$$base $(OPT_HARNESSES)/$$hyphen_base; \
			sudo cp -f $$f $(OPT_HARNESSES)/$$base; \
			sudo cp -f $$f $(OPT_HARNESSES)/$$hyphen_base; \
		fi \
	done
	@sudo chmod 755 $(ETC_HARNESSES)/*.py $(OPT_HARNESSES)/*.py 2>/dev/null || true
	@echo -e "$(GREEN)[+] Harnesses installed to $(ETC_HARNESSES) and $(OPT_HARNESSES).$(RESET)"

.PHONY: all build debug
all: build

build:
	@echo -e "$(CYAN)[+] Configuring & Building Release Targets (Jobs: $(JOBS))...$(RESET)"
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Release
	@$(MAKE) -C $(BUILD_DIR) -j$(JOBS)
	@echo -e "$(GREEN)[+] Build Complete: libics_guard.so, ics_guard_dissector_plugin.so, aryo-ics, mock_plc.$(RESET)"

debug:
	@echo -e "$(YELLOW)[+] Configuring Debug Build with Address & UB Sanitizers...$(RESET)"
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
	@$(MAKE) -C $(BUILD_DIR) -j$(JOBS)

.PHONY: test bench
test: build
	@echo -e "$(CYAN)[+] Running ics-guard Test Suite...$(RESET)"
	@cd $(BUILD_DIR) && ctest --output-on-failure
	@echo -e "$(GREEN)[+] All Unit, ABI & Pipeline Tests Passed (100%).$(RESET)"

bench: build
	@echo -e "$(CYAN)[+] Executing Cycle-Accurate Hardware Latency Benchmark (< 120 ns SLA)...$(RESET)"
	@$(BUILD_DIR)/bench_latency

.PHONY: install sync-matrix sync-web sync-all
install: build init
	@echo -e "$(CYAN)[+] Installing ics-guard System-Wide...$(RESET)"
	@sudo $(MAKE) -C $(BUILD_DIR) install
	@sudo ldconfig
	@echo -e "$(GREEN)[+] Installed /usr/local/lib/libics_guard.so, /usr/local/lib/blackbox/ics_guard_dissector_plugin.so, /usr/local/bin/aryo-ics, /usr/local/bin/mock_plc.$(RESET)"

sync-matrix: build
	@echo -e "$(CYAN)[+] Syncing libics_guard.so and mock_plc to sentinel-matrix...$(RESET)"
	@mkdir -p $(MATRIX_DIR)/shared/lib $(MATRIX_DIR)/shared/bin
	@cp -f $(BUILD_DIR)/libics_guard.so $(MATRIX_DIR)/shared/lib/
	@cp -f $(BUILD_DIR)/mock_plc $(MATRIX_DIR)/shared/bin/
	@echo -e "$(GREEN)[+] Synced to $(MATRIX_DIR)/shared/{lib,bin}$(RESET)"

sync-web:
	@echo -e "$(CYAN)[+] Synchronizing base dashboard from sentinel-nexus to sentinel-matrix...$(RESET)"
	@mkdir -p $(MATRIX_DIR)/web/plugins
	@if [ -d "$(NEXUS_DIR)/web" ]; then \
		cp -r $(NEXUS_DIR)/web/* $(MATRIX_DIR)/web/ 2>/dev/null || true; \
		mkdir -p $(MATRIX_DIR)/web/plugins; \
		echo -e "$(GREEN)[+] Base Web Dashboard Synced.$(RESET)"; \
	fi

sync-all: install sync-matrix sync-web
	@echo -e "$(GREEN)[+] Full Ecosystem Synchronization Complete.$(RESET)"

.PHONY: grid-up grid-down grid-status grid-logs
grid-up: sync-all
	@echo -e "$(CYAN)================================================================================$(RESET)"
	@echo -e "$(WHITE)  LAUNCHING PRODUCTION MASTER GRID (3 ICS SENTRY APPLIANCES + APPARATUS)       $(RESET)"
	@echo -e "$(CYAN)================================================================================$(RESET)"
	@aryo ics grid-up
	@echo -e "$(GREEN)[+] Production Master Grid Live: http://localhost:9443/$(RESET)"

grid-down:
	@echo -e "$(YELLOW)[+] Tearing down Production Master Grid...$(RESET)"
	@aryo ics grid-down || true
	@docker network prune -f >/dev/null 2>&1 || true
	@echo -e "$(GREEN)[+] Production Master Grid Dismantled.$(RESET)"

grid-status:
	@docker ps --filter "name=matrix-" --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"

grid-logs:
	@docker logs -f matrix-edge-substation-01

.PHONY: clean distclean
clean:
	@rm -rf $(BUILD_DIR)
	@echo -e "$(GREEN)[+] Clean complete.$(RESET)"

distclean: clean
	@$(MAKE) grid-down >/dev/null 2>&1 || true
	@rm -rf $(MATRIX_DIR)/.matrix_cache/docker-compose.production_master_grid.yml
	@echo -e "$(GREEN)[+] Deep clean complete.$(RESET)"