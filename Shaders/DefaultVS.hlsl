struct VS_Input {
	float3 Position : POSIITON;
	float2 UV : TEXCOORD0;
};

struct VS_Output {
	float4 Position : SV_POSITION;
	float2 UV : TEXCOORD0;
};

VS_Output main(VS_Input input)
{
    VS_Output res;

    res.Position = float4(input.Position, 1.f);
	res.UV = input.UV;
	
    return res;
}