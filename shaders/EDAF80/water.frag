#version 410

uniform vec4 color_deep;
uniform vec4 color_shallow;

uniform sampler2D normal_texture;
uniform samplerCube cubemap;

in VS_OUT {
	vec3 frag_position;
	vec2 normalCoord0;
	vec2 normalCoord1;
	vec2 normalCoord2;
	vec3 view_direction;
	vec3 T;
	vec3 B;
	vec3 N;
} fs_in;

out vec4 frag_color;

void main()
{
	// Animated Normal Mapping: Read, Remap, Superposition
	vec3 n0 = (texture(normal_texture, fs_in.normalCoord0) * 2.0 - 1.0).rgb;
	vec3 n1 = (texture(normal_texture, fs_in.normalCoord1) * 2.0 - 1.0).rgb;
	vec3 n2 = (texture(normal_texture, fs_in.normalCoord2) * 2.0 - 1.0).rgb;
	vec3 n_bump = normalize(n0 + n1 + n2); // tangent space
	mat3 TBN = mat3(normalize(fs_in.T), normalize(fs_in.B), normalize(fs_in.N));
	vec3 N = normalize(TBN * n_bump);

	float eta = 1.0 / 1.33;
	if (!gl_FrontFacing) {
		N = -N;
		eta = 1.33;
	}

	vec3 V = normalize(fs_in.view_direction);

	// Water Colour
	float facing = 1.0 - max(dot(V, N), 0.0);
	vec4 color_water = mix(color_deep, color_shallow, facing);

	// Reflection
	vec3 R = normalize(reflect(-V, N));
	vec4 reflection = texture(cubemap, R);

	// Fresnel Terms
	float R0 = 0.02037;
	float fresnel = R0 + (1.0 - R0) * pow(1.0 - dot(V, N), 5.0);
	log(fresnel);
	// Refraction
	vec4 refraction = texture(cubemap, normalize(refract(-V, N, eta)));

	// Color
	frag_color = color_water + mix(refraction, reflection, fresnel);
}
