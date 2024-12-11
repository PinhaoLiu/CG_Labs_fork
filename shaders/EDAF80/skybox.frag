#version 410

uniform samplerCube cubemap;

in VS_OUT{
	vec3 texcoord;
} fs_in;

out vec4 frag_color;

void main()
{
	frag_color = texture(cubemap, normalize(fs_in.texcoord));
}
