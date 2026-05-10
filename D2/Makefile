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
	@echo "--- parser valid ---"
	@for f in tests/valid/*.ans; do \
	    ./$(TARGET) --parse-only "$$f" && echo "PASS: $$f" || echo "FAIL: $$f"; \
	done
	@echo "--- parser invalid (should all be rejected) ---"
	@for f in tests/invalid/*.ans; do \
	    ./$(TARGET) --parse-only "$$f" && echo "FAIL (accepted): $$f" || echo "PASS (rejected): $$f"; \
	done
	@echo "--- part 2 examples (check + run) ---"
	@for f in ../D3/programs/*.ans; do \
	    ./$(TARGET) --check "$$f" && ./$(TARGET) "$$f" >/dev/null && echo "PASS: $$f" || echo "FAIL: $$f"; \
	done
	@echo "--- type errors (should be rejected before execution) ---"
	@for f in tests/type_errors/*.ans; do \
	    ./$(TARGET) --check "$$f" >/dev/null 2>&1 && echo "FAIL (accepted): $$f" || echo "PASS (rejected): $$f"; \
	done
	@echo "--- runtime errors (should fail during execution) ---"
	@for f in tests/runtime_errors/*.ans; do \
	    ./$(TARGET) "$$f" >/dev/null 2>&1 && echo "FAIL (accepted): $$f" || echo "PASS (rejected): $$f"; \
	done

clean:
	rm -f $(TARGET)
