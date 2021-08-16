ifneq ($(OS),Windows_NT)
all: server
endif
all: game shader.frag.spv shader.vert.spv

MAKEFLAGS += --no-builtin-rules -j$(shell nproc)
.SUFFIXES:

include config.mk

DEP = $(wildcard *.hpp)
GAME_SRC = game.cpp tetris.cpp client.cpp bswap.cpp message.cpp #input.cpp
GAME_OBJ = $(GAME_SRC:.cpp=.o)
GAME_DEP = $(GAME_OBJ:%.o=%.d)

SERVER_SRC = server.cpp bswap.cpp message.cpp tetris.cpp
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
SERVER_DEP = $(SERVER_OBJ:%.o=%.d)

CXXFLAGS = -g -Og -std=c++20
CXX = g++

%.d: %.cpp
	@$(CXX) $(CXXFLAGS) -MM $< -o $@

-include $(SERVER_DEP)
-include $(GAME_DEP)

%.o: %.cpp %.d
	$(CXX) $(CXXFLAGS) -c $< -o $@

game: $(GAME_OBJ) $(GAME_DEP)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(GAME_OBJ) $(LIBS) -o $@

server: $(SERVER_OBJ) $(SERVER_DEP)
	$(CXX) $(CXXFLAGS) $(SERVER_OBJ) -o $@

%.spv: %.glsl
	glslangValidator $< -V -o $@

.PHONY: clean
clean:
	rm -f *.o *.d game server
