#version 430 core

layout (location = 0) in vec3 aPos;

layout(std140) uniform Matrices
{
						// base alignment	// offset
	mat4 projection;	// 4 * 16 = 64		// 0
	mat4 view;			// 4 * 16 = 64		// 64
};

uniform mat4 model;

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0);
}