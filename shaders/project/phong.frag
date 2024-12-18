#version 410

uniform vec3 ambient_colour;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;
uniform float shininess_value;

in VS_OUT {
	vec3 fN;
	vec2 texture_coordinates;
	vec3 view_direction;
	vec3 light_direction;
	mat4 TBN;
	vec4 color;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 N = normalize(fs_in.fN);
	vec3 L = normalize(fs_in.light_direction);
	vec3 V = normalize(fs_in.view_direction);
	vec3 R = normalize(reflect(-L, N));

	vec3 ambient = ambient_colour;
	vec3 diffuse = /* diffuse_colour * */
		fs_in.color.rgb *
		max(dot(N, L), 0.0);
	vec3 specular = specular_colour *
		pow(max(dot(R, V), 0.0), shininess_value);

	frag_color.rgb = ambient + diffuse + specular;
	frag_color.a = 1.0;
}
