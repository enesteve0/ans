CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Iinclude
SRCS    = src/main.c src/lexer.c src/parser.c src/ast.c src/error.c \
          src/value.c src/checker.c src/interpreter.c
TARGET  = ans

.PHONY: all debug test clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

test: $(TARGET)
	@echo "--- valid ---"
	@for f in tests/valid/*.ans; do \
	    ./$(TARGET) "$$f" && echo "PASS: $$f" || echo "FAIL: $$f"; \
	done
	@echo "--- invalid (should all be rejected) ---"
	@for f in tests/invalid/*.ans; do \
	    ./$(TARGET) "$$f" && echo "FAIL (accepted): $$f" || echo "PASS (rejected): $$f"; \
	done

clean:
	rm -f $(TARGET)
