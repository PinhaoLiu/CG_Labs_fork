#version 410

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform mat4 normal_model_to_world;

uniform float t;
uniform vec2 amplitude;
uniform mat2 direction;
uniform vec2 frequency;
uniform vec2 phase;
uniform vec2 sharpness;

uniform vec3 camera_position;

layout (location = 0) in vec3 vertex;
layout (location = 2) in vec3 texcoords;

out VS_OUT {
	vec3 frag_position;
	vec2 normalCoord0;
	vec2 normalCoord1;
	vec2 normalCoord2;
	vec3 view_direction;
	vec3 T;
	vec3 B;
	vec3 N;
} vs_out;

float wave(float amplitude, vec2 direction, float frequency,
		   float phase, float sharpness, float time, vec2 position)
{
	return amplitude * pow(sin((position.x * direction.x + position.y * direction.y) *
						   frequency + phase * time) * 0.5 + 0.5, sharpness);
}

float pGpx(float amplitude, vec2 direction, float frequency,
		   float phase, float sharpness, float time, vec2 position)
{
	return 0.5 * sharpness * frequency * amplitude *
		   pow(sin((position.x * direction.x + position.y * direction.y) *
			   frequency + phase * time) * 0.5 + 0.5, sharpness - 1.0) *
		   cos((position.x * direction.x + position.y * direction.y) *
			   frequency + phase * time) * direction.x;
}

float pGpz(float amplitude, vec2 direction, float frequency,
		   float phase, float sharpness, float time, vec2 position)
{
	return 0.5 * sharpness * frequency * amplitude *
		   pow(sin((position.x * direction.x + position.y * direction.y) *
			   frequency + phase * time) * 0.5 + 0.5, sharpness - 1.0) *
		   cos((position.x * direction.x + position.y * direction.y) *
			   frequency + phase * time) * direction.y;
}

void main()
{
	vec3 water_position = vertex;

	water_position.y =
		wave(amplitude.x, direction[0], frequency.x, phase.x, sharpness.x, t, vertex.xz) +
		wave(amplitude.y, direction[1], frequency.y, phase.y, sharpness.y, t, vertex.xz);

	vs_out.frag_position = (vertex_model_to_world * vec4(water_position, 1.0)).xyz;

	float pHpx =
		pGpx(amplitude.x, direction[0], frequency.x, phase.x, sharpness.x, t, vertex.xz) +
		pGpx(amplitude.y, direction[1], frequency.y, phase.y, sharpness.y, t, vertex.xz);

	float pHpz =
		pGpz(amplitude.x, direction[0], frequency.x, phase.x, sharpness.x, t, vertex.xz) +
		pGpz(amplitude.y, direction[1], frequency.y, phase.y, sharpness.y, t, vertex.xz);

	vs_out.T = normalize(vec3(1.0, pHpx, 0.0));
	vs_out.B = normalize(vec3(0.0, pHpz, 1.0));
	vs_out. N = normalize(vec3(-pHpx, 1.0, -pHpz));
/*	vs_out.TBN = mat3(
		normalize(vertex_model_to_world * vec4(T, 0.0)).xyz,
		normalize(vertex_model_to_world * vec4(B, 0.0)).xyz,
		normalize(normal_model_to_world * vec4(N, 0.0)).xyz
		);*/

	vs_out.view_direction = camera_position - vs_out.frag_position;

	// Animated normal mapping: coordinates
	vec2 texScale = vec2(8.0, 4.0);
	float normalTime = mod(t, 100.0);
	vec2 normalSpeed = vec2(-0.05, 0.0);

	vs_out.normalCoord0.xy =
		texcoords.xy * texScale +
		normalTime * normalSpeed;

	vs_out.normalCoord1.xy =
		texcoords.xy * texScale * 2.0 +
		normalTime * normalSpeed * 4.0;

	vs_out.normalCoord2.xy =
		texcoords.xy * texScale * 4.0 +
		normalTime * normalSpeed * 8.0;

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(water_position, 1.0);
}
