#version 430 core

out vec4 FragColor;

struct Material {
	sampler2D texture_diffuse1;
	sampler2D texture_specular1;
	sampler2D texture_height1;
	float shininess;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform Material material;
uniform samplerCube skybox;
uniform vec3 eyePos;

float ratio = 1.0 / 1.33;

void main() 
{
	vec3 I = normalize(FragPos - eyePos);
	vec3 R = reflect(I, normalize(Normal));
	//vec3 R = refract(I, normalize(Normal), ratio);
	vec4 reflectionColor = vec4(texture(skybox, R).rgb, 1.0);

	vec4 materialColor = texture(material.texture_diffuse1, TexCoords);
	vec4 specularColor = texture(material.texture_specular1, TexCoords);

	//FragColor = (1 - materialColor.a) * materialColor + materialColor.a * reflectionColor;
	FragColor = materialColor + mix(reflectionColor, specularColor, 0.3);
}