all: server
#all: game shader.frag.spv shader.vert.spv

MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include config.mk

DEP = $(wildcard *.hpp)
GAME_SRC = game.cpp tetris.cpp input.cpp
GAME_OBJ = $(GAME_SRC:.cpp=.o)
GAME_DEP = $(GAME_OBJ:%.o=%.d)

SERVER_SRC = server.cpp bswap.cpp
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)
SERVER_DEP = $(SERVER_OBJ:%.o=%.d)

CXXFLAGS = -g -Og -std=c++20

%.d: %.cpp
	g++ $(CXXFLAGS) -MM $< -o $@

-include $(SERVER_DEP)
-include $(GAME_DEP)

%.o: %.cpp %.d
	g++ $(CXXFLAGS) -c $< -o $@

game: $(GAME_OBJ) $(GAME_DEP)
	g++ $(CXXFLAGS) $(LDFLAGS) $(LIBS) $(GAME_OBJ) -o $@

server: $(SERVER_OBJ) $(SERVER_DEP)
	g++ $(CXXFLAGS) $(SERVER_OBJ) -o $@

%.spv: %.glsl
	glslangValidator $< -V -o $@
