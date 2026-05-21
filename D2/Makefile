CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Iinclude
SRCS    = src/main.c src/lexer.c src/parser.c src/ast.c src/error.c \
          src/value.c src/checker.c src/interpreter.c
TARGET  = ans

D3DIR   = ../D3

.PHONY: all debug test clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# -----------------------------------------------------------------------
# test: run all categories; exit 1 if any failure.
# D3 categories require ../D3/ to exist. Missing D3 assets are a failure
# because Part 2 requires end-to-end evidence for the D3 programs.
# -----------------------------------------------------------------------
test: $(TARGET)
	@PASS=0; FAIL=0; \
	\
	echo "=== Parser: valid programs (expect exit 0) ==="; \
	for f in tests/valid/*.ans; do \
	    if ./$(TARGET) "$$f" >/dev/null 2>&1; then \
	        echo "  PASS: $$f"; PASS=$$((PASS+1)); \
	    else \
	        echo "  FAIL: $$f"; FAIL=$$((FAIL+1)); \
	    fi; \
	done; \
	\
	echo "=== Parser: invalid programs (expect exit 1) ==="; \
	for f in tests/invalid/*.ans; do \
	    if ./$(TARGET) "$$f" >/dev/null 2>&1; then \
	        echo "  FAIL (accepted): $$f"; FAIL=$$((FAIL+1)); \
	    else \
	        echo "  PASS (rejected): $$f"; PASS=$$((PASS+1)); \
	    fi; \
	done; \
	\
	echo "=== Type checker: valid D2 programs (--check, expect exit 0) ==="; \
	for f in tests/check-valid/*.ans; do \
	    if ./$(TARGET) --check "$$f" >/dev/null 2>&1; then \
	        echo "  PASS: $$f"; PASS=$$((PASS+1)); \
	    else \
	        echo "  FAIL: $$f"; FAIL=$$((FAIL+1)); \
	    fi; \
	done; \
	\
	echo "=== Type checker: invalid D2 programs (--check, expect exit 1) ==="; \
	for f in tests/check-invalid/*.ans; do \
	    if ./$(TARGET) --check "$$f" >/dev/null 2>&1; then \
	        echo "  FAIL (accepted): $$f"; FAIL=$$((FAIL+1)); \
	    else \
	        echo "  PASS (rejected): $$f"; PASS=$$((PASS+1)); \
	    fi; \
	done; \
	\
	if [ -d "$(D3DIR)/programs" ]; then \
	    echo "=== D3 example programs (--run, expect exit 0) ==="; \
	    for f in $(D3DIR)/programs/*.ans; do \
	        if ./$(TARGET) --run "$$f" >/dev/null 2>&1; then \
	            echo "  PASS: $$f"; PASS=$$((PASS+1)); \
	        else \
	            echo "  FAIL: $$f"; FAIL=$$((FAIL+1)); \
	        fi; \
	    done; \
	else \
	    echo "=== D3 example programs: $(D3DIR)/programs not found — FAIL ==="; \
	    FAIL=$$((FAIL+1)); \
	fi; \
	\
	if [ -d "$(D3DIR)/malformed" ]; then \
	    echo "=== D3 malformed programs (parse, expect exit 1) ==="; \
	    for f in $(D3DIR)/malformed/*.ans; do \
	        if ./$(TARGET) "$$f" >/dev/null 2>&1; then \
	            echo "  FAIL (accepted): $$f"; FAIL=$$((FAIL+1)); \
	        else \
	            echo "  PASS (rejected): $$f"; PASS=$$((PASS+1)); \
	        fi; \
	    done; \
	else \
	    echo "=== D3 malformed programs: $(D3DIR)/malformed not found — FAIL ==="; \
	    FAIL=$$((FAIL+1)); \
	fi; \
	\
	if [ -d "$(D3DIR)/semantic" ]; then \
	    echo "=== Type-checker negative tests (--check, expect exit 1) ==="; \
	    for f in $(D3DIR)/semantic/sem*.ans; do \
	        if ./$(TARGET) --check "$$f" >/dev/null 2>&1; then \
	            echo "  FAIL (accepted): $$f"; FAIL=$$((FAIL+1)); \
	        else \
	            echo "  PASS (rejected): $$f"; PASS=$$((PASS+1)); \
	        fi; \
	    done; \
	    echo "=== Runtime-error tests (--run, expect exit 1) ==="; \
	    for f in $(D3DIR)/semantic/rt*.ans; do \
	        if ./$(TARGET) --run "$$f" >/dev/null 2>&1; then \
	            echo "  FAIL (no error raised): $$f"; FAIL=$$((FAIL+1)); \
	        else \
	            echo "  PASS (error raised): $$f"; PASS=$$((PASS+1)); \
	        fi; \
	    done; \
	else \
	    echo "=== Type-checker/runtime tests: $(D3DIR)/semantic not found — FAIL ==="; \
	    FAIL=$$((FAIL+1)); \
	fi; \
	\
	echo ""; \
	echo "Results: $$PASS passed, $$FAIL failed."; \
	[ "$$FAIL" -eq 0 ]

clean:
	rm -f $(TARGET)
