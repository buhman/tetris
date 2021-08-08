all: a.out shader.frag.spv shader.vert.spv

DEP = $(wildcard *.h)
SRC = game.cpp tetris.cpp
OBJ = $(SRC:.cpp=.o)
LIBS = -lvulkan -lglfw

%.o: %.cpp
	g++ -g -Og -std=c++17 -c $<

a.out: $(OBJ) $(DEP)
	g++ -g -Og -std=c++17 -o $@ $(LIBS) $(OBJ)

shader.frag.spv: shader.frag.glsl
	glslangValidator shader.frag.glsl -V -o shader.frag.spv

shader.vert.spv: shader.vert.glsl
	glslangValidator shader.vert.glsl -V -o shader.vert.spv
