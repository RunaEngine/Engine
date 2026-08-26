
RWTexture2D<float4> SrcMip : register(u0, space0);
RWTexture2D<float4> DstMip : register(u1, space0);

[shader("compute")]
[numthreads(16, 16, 1)]
void cs_main(uint3 gid : SV_DispatchThreadID)
{
    uint2 dstPos = gid.xy;
    uint2 srcPos = dstPos * 2;

    uint dstW, dstH;
    DstMip.GetDimensions(dstW, dstH);
    if (dstPos.x >= dstW || dstPos.y >= dstH)
        return;

    float4 t00 = SrcMip[srcPos];
    float4 t01 = SrcMip[srcPos + uint2(0, 1)];
    float4 t10 = SrcMip[srcPos + uint2(1, 0)];
    float4 t11 = SrcMip[srcPos + uint2(1, 1)];

    DstMip[dstPos] = (t00 + t01 + t10 + t11) * 0.25;
}