#version 410

struct ViewProjTransforms
{
	mat4 view_projection;
	mat4 view_projection_inverse;
};

layout (std140) uniform CameraViewProjTransforms
{
	ViewProjTransforms camera;
};

layout (std140) uniform LightViewProjTransforms
{
	ViewProjTransforms lights[4];
};

uniform int light_index;

uniform sampler2D depth_texture;
uniform sampler2D normal_texture;
uniform sampler2D shadow_texture;

uniform vec2 inverse_screen_resolution;

uniform vec3 camera_position;

uniform vec3 light_color;
uniform vec3 light_position;
uniform vec3 light_direction;
uniform float light_intensity;
uniform float light_angle_falloff;

layout (location = 0) out vec4 light_diffuse_contribution;
layout (location = 1) out vec4 light_specular_contribution;


void main()
{
	vec2 shadowmap_texel_size = 1.0f / textureSize(shadow_texture, 0);

	vec2 tex_coords = gl_FragCoord.xy * inverse_screen_resolution;
	vec3 normal = normalize(texture(normal_texture, tex_coords).xyz * 2.0 - 1.0);
	float depth = texture(depth_texture, tex_coords).r;
	vec4 world_pos = camera.view_projection_inverse * (vec4(tex_coords, depth, 1.0) * 2.0 - 1.0);
	world_pos /= world_pos.w;

	vec3 L = normalize(light_position - world_pos.xyz);
	float diffuse = max(dot(normal, L), 0.0);

	vec3 V = normalize(camera_position - world_pos.xyz);
	float specular = pow(max(dot(reflect(-L, normal), V), 0.0), 10.0);

	float theta = acos(dot(-L, normalize(light_direction)));
	float d = length(light_position - world_pos.xyz);
	float I = light_intensity * (1.0 - smoothstep(0.0, light_angle_falloff, theta)) / (1.0 + pow(d, 2.0));
	diffuse *= I;
	specular *= I;

	vec4 shadow_pos = lights[light_index].view_projection * world_pos;
	shadow_pos /= shadow_pos.w;
	shadow_pos.xyz = (shadow_pos.xyz + 1.0) / 2.0;
	float p = 0.0;
	int n = 2;
	for (int dx = -n; dx <= n; dx++) {
		for (int dy = -n; dy <= n; dy++) {
			float shadow_depth = texture(shadow_texture, shadow_pos.xy + vec2(dx, dy) * shadowmap_texel_size).r + 0.00001;
			if (shadow_pos.z > shadow_depth) p += 1.0;
		}
	}
	p /= pow(2.0 * n + 1.0, 2.0);
	diffuse *= 1.0 - p;
	specular *= 1.0 - p;

	light_diffuse_contribution = vec4(light_color * diffuse, 1.0);
	light_specular_contribution = vec4(light_color * specular, 1.0);
}
