ifeq ($(OS),Windows_NT)
LIBS = -lvulkan-1 -lglfw3 -lws2_32
LDFLAGS = -L/mingw64/bin
else
LIBS = -lvulkan -lglfw
LDFLAGS = -pthread
endif
