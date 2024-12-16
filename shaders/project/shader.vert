#version 410

layout (location = 0) in vec3 vertex;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform vec3 velocity;

out vec4 color;

void main()
{
	float speed = length(velocity);
	float k = 100000000.0;
	float t = log(1.0 + k * speed) / log(1.0 + k * 200.0);
	t = speed;

	if (t < 0.5)
		color = mix(vec4(0.0, 0.0, 1.0, 1.0), vec4(0.0, 1.0, 0.0, 1.0), t * 2.0);
	if (t >= 0.5)
		color = mix(vec4(0.0, 1.0, 0.0, 1.0), vec4(1.0, 0.0, 0.0, 1.0), t * 2.0 - 1.0);

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}
