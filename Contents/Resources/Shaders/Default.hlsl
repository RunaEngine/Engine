struct CameraUniform {
    float4x4 view_proj;
};

ConstantBuffer<CameraUniform> camera : register(b0, space1);

struct VertexInput {
    float3 position   : POSITION;
    float2 tex_coords : TEXCOORD0;
};

struct VertexOutput {
    float4 clip_position : SV_Position;
    float2 tex_coords    : TEXCOORD0;
};

VertexOutput vs_main(VertexInput model) {
    VertexOutput out_struct;

    out_struct.tex_coords = model.tex_coords;

    out_struct.clip_position = mul(float4(model.position, 1.0), camera.view_proj);;

    return out_struct;
}

Texture2D t_diffuse : register(t0);
SamplerState s_diffuse : register(s1);

float4 fs_main(VertexOutput input) : SV_TARGET
{
    return t_diffuse.Sample(s_diffuse, input.tex_coords);
}