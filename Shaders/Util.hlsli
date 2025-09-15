

float3 sRGBToLinear(float3 c) {
	//return float3(pow(c, float3(2.2f)));
    float3 x = c / 12.92f;
    float3 y = pow(max(( c + 0.055f) / 1.055f, 0.0f), 2.4f);

    float3 clr = c;
    clr.r = c.r <= 0.04045f ? x.r : y.r;
    clr.g = c.g <= 0.04045f ? x.g : y.g;
    clr.b = c.b <= 0.04045f ? x.b : y.b;

    return clr;
}

float4 sRGBToLinear(float4 c) {
	return float4(sRGBToLinear(c.rgb), c.a);
}

float3 LinearTosRGB(float3 c) {
	//return float3(pow(c, float3(1.f/2.2f)));

    float3 x = c * 12.92f;
    float3 y = 1.055f * pow(saturate(c), 1.0f / 2.4f) - 0.055f;

    float3 clr = c;
    clr.r = c.r < 0.0031308f ? x.r : y.r;
    clr.g = c.g < 0.0031308f ? x.g : y.g;
    clr.b = c.b < 0.0031308f ? x.b : y.b;

    return clr;
}

float4 LinearTosRGB(float4 c) {
	return float4(LinearTosRGB(c.rgb), c.a);
}

// float3 tonemap(float3 c)
// {
// 	float3 x = max(float3(0.f), c - 0.004);
// 	return (x * (6.2 * x + .5)) / (x * (6.2 * x + 1.7) + 0.06);
// }

// ACES approx tonemapping
float3 tonemap(float3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d ) + e));
}

float3 GetCascadeColorTint(uint cascadeIndex) {
	switch(cascadeIndex) {
		case 0:
			return float3(1.f, 0.25f, 0.25f);
		case 1:
			return float3(0.25f, 1.0f, 0.25f);
		case 2:
			return float3(0.25f, 0.25f, 1.0f);
		case 3:
			return float3(1.f, 1.f, 0.25f);
	}

	return float3(0.f);
}

float Dither2x2(int2 p) {
	float d = 0.0;
	
	if((p.x & 1) != (p.y & 1))
		d += 2.0;
	if((p.y & 1) == 1)
		d += 1.0;
	
	d *= 0.25;
	
	return d;
}

float Dither4x4(int2 p) {
	float d = Dither2x2(p);
	d = d * 0.25 + Dither2x2(p >> 1);
	return d;
}