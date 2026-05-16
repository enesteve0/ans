CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Iinclude
SRCS    = src/main.c src/lexer.c src/parser.c src/ast.c src/error.c src/semantics.c
TARGET  = ans

.PHONY: all debug test clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

test: $(TARGET)
	@status=0; \
	echo "--- parser valid ---"; \
	for f in tests/valid/*.ans; do \
	    if [ ! -e "$$f" ]; then echo "FAIL: missing tests/valid/*.ans"; status=1; break; fi; \
	    ./$(TARGET) --parse-only "$$f" && echo "PASS: $$f" || { echo "FAIL: $$f"; status=1; }; \
	done; \
	echo "--- parser invalid (should all be rejected) ---"; \
	for f in tests/invalid/*.ans; do \
	    if [ ! -e "$$f" ]; then echo "FAIL: missing tests/invalid/*.ans"; status=1; break; fi; \
	    ./$(TARGET) --parse-only "$$f" && { echo "FAIL (accepted): $$f"; status=1; } || echo "PASS (rejected): $$f"; \
	done; \
	echo "--- part 2 examples (check + run) ---"; \
	for f in ../D3/programs/*.ans; do \
	    if [ ! -e "$$f" ]; then echo "FAIL: missing ../D3/programs/*.ans"; status=1; break; fi; \
	    ./$(TARGET) --check "$$f" && ./$(TARGET) "$$f" >/dev/null && echo "PASS: $$f" || { echo "FAIL: $$f"; status=1; }; \
	done; \
	echo "--- type errors (should be rejected before execution) ---"; \
	for f in tests/type_errors/*.ans; do \
	    if [ ! -e "$$f" ]; then echo "FAIL: missing tests/type_errors/*.ans"; status=1; break; fi; \
	    ./$(TARGET) --check "$$f" >/dev/null 2>&1 && { echo "FAIL (accepted): $$f"; status=1; } || echo "PASS (rejected): $$f"; \
	done; \
	echo "--- runtime errors (should fail during execution) ---"; \
	for f in tests/runtime_errors/*.ans; do \
	    if [ ! -e "$$f" ]; then echo "FAIL: missing tests/runtime_errors/*.ans"; status=1; break; fi; \
	    ./$(TARGET) "$$f" >/dev/null 2>&1 && { echo "FAIL (accepted): $$f"; status=1; } || echo "PASS (rejected): $$f"; \
	done; \
	exit $$status

clean:
	rm -f $(TARGET)
