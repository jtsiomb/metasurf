varying vec3 vpos, ldir, norm;

void main()
{
	gl_Position = ftransform();

	vpos = (gl_ModelViewMatrix * gl_Vertex).xyz;
	norm = gl_NormalMatrix * gl_Normal;
	ldir = gl_LightSource[0].position.xyz;
}
