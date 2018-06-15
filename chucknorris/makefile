RM=rm -rf
CC=gcc

C_SRCS=$(shell find . -iname "*.c" | tr '\n' ' ')
H_SRCS=$(shell find . -iname "*.h" | tr '\n' ' ')
OBJS=$(C_SRCS:./%.c=build/%.o)

# Clean and compile .so
all: build/libchucknorris.so

create-dirs:
	mkdir -p build/chucknorris/

build/libchucknorris.so: create-dirs $(OBJS)
	$(CC) -shared -o "build/libchucknorris.so" $(OBJS)

build/chucknorris/%.o: chucknorris/%.c
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

# Add debug parameters and compile
debug: CC += -DDEBUG -g
debug: all

# Clean build files
clean:
	$(RM) build

install: all
	cp -u build/libchucknorris.so /usr/lib
	cp --parents -u $(H_SRCS) /usr/include

uninstall:
	rm -f /usr/lib/libchucknorris.so
	rm -rf /usr/include/chucknorris

.PHONY: all create-dirs clean install uninstall