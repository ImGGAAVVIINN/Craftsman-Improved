#if __VERSION__ >= 300

uniform highp vec2 ATLAS_FACE_SIZE;
uniform highp vec3 TEXTURE_DIMENSIONS;

highp vec2 clampUVsForMipLevel(in highp vec2 uv, in highp float mipLevel, in highp float textureWidth, in highp float textureHeight)
{
	highp vec2 uvClamped;

	// Clamp UVs to atlas face so that neighboring faces don't bleed across
	highp vec2 numAtlasFaces = vec2(textureWidth, textureHeight) / ATLAS_FACE_SIZE;
	highp vec2 faceUV = fract(uv * numAtlasFaces);
	uvClamped = (uv * numAtlasFaces) - faceUV;
	highp vec2 halfTexel = vec2(.5f, .5f) / (ATLAS_FACE_SIZE / pow(2.0f, mipLevel));
	faceUV = clamp(faceUV, halfTexel, vec2(1.0f, 1.0f) - halfTexel);
	uvClamped += faceUV;
	uvClamped /= numAtlasFaces;

	return uvClamped;
}

vec4 texture2D_AA(in sampler2D tex, in highp vec2 uv)
{
	// Texture antialiasing
	//
	// The texture coordinates are modified so that the bilinear filter will be one pixel wide instead of one texel wide. 

	// Get the UV deltas
	highp vec2 dUVdx = dFdx(uv) * TEXTURE_DIMENSIONS.xy;
	highp vec2 dUVdy = dFdy(uv) * TEXTURE_DIMENSIONS.xy;
	highp vec2 dU = vec2(dUVdx.x, dUVdy.x);
	highp vec2 dV = vec2(dUVdx.y, dUVdy.y);

	highp float duUV = sqrt(dot(dU, dU));
	highp float dvUV = sqrt(dot(dV, dV));

	// Determine mip map LOD
	highp float d = max(dot(dUVdx, dUVdx), dot(dUVdy, dUVdy));
	highp float mipLevel = .5f * log2(d);
	mipLevel = mipLevel + .5f;	// Mip bias to reduce aliasing
	mipLevel = clamp(mipLevel, 0.0f, TEXTURE_DIMENSIONS.z);

	highp vec2 uvModified;
	if( mipLevel >= 1.0f)
	{
		uvModified = uv;
	}
	else
	{
		// First scale the uv so that each texel has a uv range of [0,1]
		highp vec2 texelUV = fract(uv * TEXTURE_DIMENSIONS.xy);

		// Initially set uvModified to the floor of the texel position
		uvModified = (uv * TEXTURE_DIMENSIONS.xy) - texelUV;

		// Modify the texelUV to push the uvs toward the edges.
		//          |                 |       |                   |
		//          |         _/      |       |           /       |
		//  Change  | U     _/        |  to   | U     ___/        |
		//          |     _/          |       |     /             |
		//          |    /            |       |    /              |
		//          |         X       |       |         X         |
		highp float scalerU = 1.0f / (duUV);
		highp float scalerV = 1.0f / (dvUV);
		highp vec2 scaler = max(vec2(scalerU, scalerV), 1.0f);
		texelUV = clamp(texelUV * scaler, 0.0f, .5f) + clamp(texelUV*scaler - (scaler - .5f), 0.0f, .5f);
		uvModified += texelUV;
		uvModified /= TEXTURE_DIMENSIONS.xy;
	}
#ifdef ATLAS_TEXTURE
	//Clamp UVs to atlas face

	highp float mipLevel0 = clamp(floor(mipLevel), 0.0f, TEXTURE_DIMENSIONS.z);
	highp float mipLevel1 = clamp(floor(mipLevel+1.0f), 0.0f, TEXTURE_DIMENSIONS.z);

	highp vec2 uvLOD0 = clampUVsForMipLevel(uvModified, mipLevel0, TEXTURE_DIMENSIONS.x, TEXTURE_DIMENSIONS.y);
	highp vec2 uvLOD1 = clampUVsForMipLevel(uvModified, mipLevel1, TEXTURE_DIMENSIONS.x, TEXTURE_DIMENSIONS.y);

	vec4 diffuse0 = textureLod(tex, uvLOD0, mipLevel0);
	vec4 diffuse1 = textureLod(tex, uvLOD1, mipLevel1);

	vec4 diffuse = mix(diffuse0, diffuse1, fract(mipLevel));
#else
	vec4 diffuse = texture2D(tex, uvModified);
#endif
	return diffuse;

}

#endif //__VERSION__ >= 300
