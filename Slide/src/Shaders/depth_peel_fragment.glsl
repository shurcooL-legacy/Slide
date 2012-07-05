uniform sampler2DRect DepthTex;

void main()
{
	// Bit-exact comparison between FP32 z-buffer and fragment depth
	float frontDepth = texture2DRect(DepthTex, gl_FragCoord.xy).x;
	if (frontDepth/* + 0.00000003*/ >= gl_FragCoord.z || 0.0 == frontDepth) {
		discard;
	}

	// Shade all the fragments behind the z-buffer
	gl_FragColor = vec4(1);
}
