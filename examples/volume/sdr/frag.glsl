varying vec3 vpos, ldir, norm;

void main()
{
	vec3 n = normalize(norm);
	vec3 l = normalize(ldir);
	vec3 v = -normalize(vpos);
	vec3 h = normalize(v + l);

	const vec3 kd = vec3(0.87, 0.82, 0.74);

	float diff = abs(dot(n, l));
	float spec = pow(abs(dot(n, h)), 60.0);

	vec3 dcol = kd * diff;
	vec3 scol = vec3(0.8, 0.8, 0.8) * spec;

	gl_FragColor = vec4(dcol + scol, 1.0);
}
