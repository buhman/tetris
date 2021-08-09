ifeq ($(OS),Windows_NT)
LIBS = -lvulkan-1 -lglfw3
LDFLAGS = -L/mingw64/bin
else
LIBS = -lvulkan -lglfw
LDFLAGS =
endif
