#include <stdlib.h>

#ifndef __APPLE__
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif

#include "metasurf.h"

float eval(struct metasurface *ms, float x, float y, float z);
void vertex(struct metasurface *ms, float x, float y, float z);
void normal(struct metasurface *ms, float x, float y, float z);
void disp(void);
void reshape(int x, int y);
void keyb(unsigned char key, int x, int y);

struct metasurface *ms;

int main(int argc, char **argv)
{
	float ldir[] = {-0.3, 0.3, 1, 0};

	glutInitWindowSize(800, 600);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow("metasurf example: simple");

	glutDisplayFunc(disp);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyb);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, ldir);

	glEnable(GL_NORMALIZE);

	ms = msurf_create();
	/* consider anything below the threshold (0 by default) to be inside */
	msurf_set_inside(ms, MSURF_LESS);
	/* set the evaluation callback */
	msurf_eval_func(ms, eval);
	msurf_vertex_func(ms, vertex);
	msurf_normal_func(ms, normal);
	/* slightly increase the bounds to avoid clipping the unit sphere */
	msurf_set_bounds(ms, -1.1, -1.1, -1.1, 1.1, 1.1, 1.1);

	glutMainLoop();
	return 0;
}

/* the unit sphere is implicitly defined as the locus of points in R3 satisfying
 * the equation: x^2 + y^2 + z^2 - 1 = 0
 */
float eval(struct metasurface *ms, float x, float y, float z)
{
	return (x * x + y * y + z * z) - 1.0;
}

void vertex(struct metasurface *ms, float x, float y, float z)
{
	/* pass any vertices generated directly to OpenGL */
	glVertex3f(x, y, z);
}

void normal(struct metasurface *ms, float x, float y, float z)
{
	/* pass any normals generated directly to OpenGL */
	glNormal3f(x, y, z);
}

void disp(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -3);

	glBegin(GL_TRIANGLES);
	msurf_polygonize(ms);
	glEnd();

	glutSwapBuffers();
}

void reshape(int x, int y)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (float)x / (float)y, 0.5, 500.0);
}

void keyb(unsigned char key, int x, int y)
{
	switch(key) {
	case 27:
		exit(0);

	case 'w':
		{
			static int wire;
			wire = !wire;
			glPolygonMode(GL_FRONT_AND_BACK, wire ? GL_LINE : GL_FILL);
			glutPostRedisplay();
		}
		break;

	default:
		break;
	}
}
