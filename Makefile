CC       := gcc

CPPFLAGS := -Iinclude # -MMD -MP
CFLAGS   := -Wall -g -O0
LDLIBS   := 

EXE := fsw.exe

.PHONY: all clean
all: $(EXE)

fsw.exe: src/main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -rf $(EXE) *.d