#version 410

uniform vec3 ambient_colour;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;
uniform float shininess_value;

uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;
uniform sampler2D normal_texture;

uniform int use_normal_mapping;

in VS_OUT {
	vec3 fN;
	vec2 texture_coordinates;
	vec3 view_direction;
	vec3 light_direction;
	mat4 TBN;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 N = normalize(fs_in.fN);

	if (use_normal_mapping == 1)
		N = normalize((fs_in.TBN * vec4(texture(normal_texture, fs_in.texture_coordinates).rgb * 2.0 - 1.0, 0.0)).xyz);

	vec3 L = normalize(fs_in.light_direction);
	vec3 V = normalize(fs_in.view_direction);
	vec3 R = normalize(reflect(-L, N));

	vec3 ambient = ambient_colour;
	vec3 diffuse = /* diffuse_colour * */
		max(dot(N, L), 0.0) *
		texture(diffuse_texture, fs_in.texture_coordinates).rgb;
	vec3 specular = /* specular_colour * */
		pow(max(dot(R, V), 0.0), shininess_value) *
		texture(specular_texture, fs_in.texture_coordinates).rgb;

	frag_color.rgb = ambient + diffuse + specular;
	frag_color.a = 1.0;
}
