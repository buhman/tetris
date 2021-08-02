all: a.out shader.frag.spv shader.vert.spv

a.out: game.cpp
	g++ -g -std=c++17 -lvulkan -lglfw game.cpp

shader.frag.spv: shader.frag.glsl
	glslangValidator shader.frag.glsl -V -o shader.frag.spv

shader.vert.spv: shader.vert.glsl
	glslangValidator shader.vert.glsl -V -o shader.vert.spv
