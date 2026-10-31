SRCS=$(wildcard src/*.c)

OBJS=$(patsubst src/%.c,dist/%.o,$(SRCS))


default: dist/server


dist/%.o: src/%.c | dist/
	$(CC) -c $< -o $@ -MMD -MD

dist/server: $(OBJS)
	$(CC) $^ -o $@

dist/:
	mkdir dist


clean:
	$(RM) -rf dist/


include $(wildcard dist/*.d)

.PHONY: clean default
