[[vk::binding(0, 0)]] Texture2D t_diffuse : register(t0);
[[vk::binding(1, 0)]] SamplerState s_diffuse : register(s1);

struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 tex_coords : TEXCOORD0;
};

float4 fs_main(VertexOutput input) : SV_TARGET
{
    return t_diffuse.Sample(s_diffuse, input.tex_coords);
}