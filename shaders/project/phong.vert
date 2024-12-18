#version 410

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform vec3 light_position;
uniform vec3 camera_position;

uniform vec3 velocity;

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texture_coordinates;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

out VS_OUT {
	vec3 fN;
	vec2 texture_coordinates;
	vec3 view_direction;
	vec3 light_direction;
	mat4 TBN;
	vec4 color;
} vs_out;

void main()
{
	vec3 world_position = (vertex_model_to_world * vec4(vertex, 1.0)).xyz;

	vs_out.fN = (normal_model_to_world * vec4(normal, 0.0)).xyz;

	vs_out.texture_coordinates = texture_coordinates.xy;

	vs_out.view_direction = camera_position - world_position;
	vs_out.light_direction = light_position - world_position;

	vec3 t = (vertex_model_to_world * vec4(tangent, 0.0)).xyz;
	vec3 b = (vertex_model_to_world * vec4(binormal, 0.0)).xyz;
	vec3 n = (normal_model_to_world * vec4(normal, 0.0)).xyz;

	vs_out.TBN = mat4(mat3(t, b, n));

	float speed = length(velocity);
	float x = speed;

	if (x < 0.5)
		vs_out.color = mix(vec4(0.1, 0.1, 1.0, 1.0), vec4(0.1, 1.0, 0.1, 1.0), x * 2.0);
	if (x >= 0.5)
		vs_out.color = mix(vec4(0.1, 1.0, 0.1, 1.0), vec4(1.0, 0.1, 0.1, 1.0), x * 2.0 - 1.0);

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}
