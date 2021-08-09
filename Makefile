all: a.out shader.frag.spv shader.vert.spv

include config.mk

DEP = $(wildcard *.hpp)
SRC = game.cpp tetris.cpp stb_image.c
OBJ = $(SRC:.cpp=.o)

%.o: %.cpp
	g++ -g -Og -std=c++17 -c $<

a.out: $(OBJ) $(DEP)
	g++ -g -Og -std=c++17 $(LDFLAGS) $(LIBS) $(OBJ) -o $@

shader.frag.spv: shader.frag.glsl
	glslangValidator shader.frag.glsl -V -o shader.frag.spv

shader.vert.spv: shader.vert.glsl
	glslangValidator shader.vert.glsl -V -o shader.vert.spv
