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

    out_struct.clip_position = mul(camera.view_proj, float4(model.position, 1.0));

    return out_struct;
}