#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#ifndef NO_SHADERS
#include <GL/glew.h>
#include "sdr.h"
#endif

#ifndef __APPLE__
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif

#include "cam.h"
#include "metasurf.h"

#define RES		38

struct metaball {
	float energy;
	float x, y, z;
} mball[] = {
	{1.0, 0, 0, 0},
	{0.25, 0.45, 0, 0.25},
	{0.15, -0.3, 0.2, 0.1}
};

int num_mballs = sizeof mball / sizeof *mball;

float eval(struct metasurface *ms, float x, float y, float z);
void vertex(struct metasurface *ms, float x, float y, float z);
void render(void);
void disp(void);
void reshape(int x, int y);
void keyb(unsigned char key, int x, int y);
void mouse(int bn, int state, int x, int y);
void motion(int x, int y);
void sball_button(int bn, int state);
void sball_motion(int x, int y, int z);
int parse_args(int argc, char **argv);

int stereo, fullscreen;
int orig_xsz, orig_ysz;

struct metasurface *msurf;
float threshold = 12;
#ifndef NO_SHADERS
unsigned int sdr;
#endif
int bidx = 1;

int main(int argc, char **argv)
{
	float amb[] = {0, 0, 0, 0};
	float lpos[] = {-0.2, 0.2, 1, 0};

	glutInitWindowSize(1280, 720);
	glutInit(&argc, argv);

	if(parse_args(argc, argv) == -1) {
		return 1;
	}
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE | (stereo ? GLUT_STEREO : 0));
	glutCreateWindow("metasurf");

	orig_xsz = glutGet(GLUT_WINDOW_WIDTH);
	orig_ysz = glutGet(GLUT_WINDOW_HEIGHT);

	if(fullscreen) {
		glutFullScreen();
	}

	glutDisplayFunc(disp);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyb);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutSpaceballButtonFunc(sball_button);
	glutSpaceballMotionFunc(sball_motion);

#ifndef NO_SHADERS
	glewInit();
	if(!(sdr = create_program_load("sdr/vert.glsl", "sdr/frag.glsl"))) {
		return 1;
	}
#endif

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, lpos);

	glEnable(GL_NORMALIZE);

	cam_focus_dist(2.0);
	cam_clip(0.1, 200.0);
	cam_rotate(0, 0);
	cam_dolly(2);

	msurf = msurf_create();
	msurf_eval_func(msurf, eval);
	msurf_vertex_func(msurf, vertex);
	msurf_set_threshold(msurf, threshold);
	msurf_set_resolution(msurf, RES, RES, RES);
	msurf_set_bounds(msurf, -1, -1, -1, 1, 1, 1);

	glClearColor(0.8, 0.8, 0.8, 1.0);

	glutMainLoop();
	return 0;
}

float eval(struct metasurface *ms, float x, float y, float z)
{
	int i;
	float val = 0.0f;

	for(i=0; i<num_mballs; i++) {
		float dx = mball[i].x - x;
		float dy = mball[i].y - y;
		float dz = mball[i].z - z;
		float dist_sq = dx * dx + dy * dy + dz * dz;

		if(dist_sq < 1e-6) {
			val += 100.0;
		} else {
			val += mball[i].energy / dist_sq;
		}
	}
	return val;
}

void vertex(struct metasurface *ms, float x, float y, float z)
{
	const float dt = 0.001;
	float dfdx = eval(ms, x - dt, y, z) - eval(ms, x + dt, y, z);
	float dfdy = eval(ms, x, y - dt, z) - eval(ms, x, y + dt, z);
	float dfdz = eval(ms, x, y, z - dt) - eval(ms, x, y, z + dt);

	glNormal3f(dfdx, dfdy, dfdz);
	glVertex3f(x, y, z);
}

void render(void)
{
	float kd[] = {0.7, 0.28, 0.2, 1.0};
	float ks[] = {0.9, 0.9, 0.9, 1.0};

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, kd);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ks);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 60.0);

#ifndef NO_SHADERS
	bind_program(sdr);
#endif

	glBegin(GL_TRIANGLES);
	msurf_polygonize(msurf);
	glEnd();

	assert(glGetError() == GL_NO_ERROR);
}

void disp(void)
{
	if(stereo) {
		glDrawBuffer(GL_BACK_LEFT);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	cam_stereo_proj_matrix(stereo ? CAM_LEFT : CAM_CENTER);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	cam_stereo_view_matrix(stereo ? CAM_LEFT : CAM_CENTER);

	render();

	if(stereo) {
		glDrawBuffer(GL_BACK_RIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		cam_stereo_proj_matrix(CAM_RIGHT);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		cam_stereo_view_matrix(CAM_RIGHT);

		render();
	}
	glutSwapBuffers();
}

void reshape(int x, int y)
{
	glViewport(0, 0, x, y);
	cam_aspect((float)x / (float)y);
}

void keyb(unsigned char key, int x, int y)
{
	static int wire;

	switch(key) {
	case 27:
		exit(0);

	case 'f':
		fullscreen = !fullscreen;
		if(fullscreen) {
			glutFullScreen();
		} else {
			glutReshapeWindow(orig_xsz, orig_ysz);
		}
		break;

	case 's':
		stereo = !stereo;
		glutPostRedisplay();
		break;

	case 'w':
		wire = !wire;
		glPolygonMode(GL_FRONT_AND_BACK, wire ? GL_LINE : GL_FILL);
		glutPostRedisplay();
		break;

	case '=':
		threshold += 0.05;
		msurf_set_threshold(msurf, threshold);
		printf("threshold: %f\n", threshold);
		glutPostRedisplay();
		break;

	case '-':
		threshold -= 0.05;
		msurf_set_threshold(msurf, threshold);
		printf("threshold: %f\n", threshold);
		glutPostRedisplay();
		break;

	case ' ':
		bidx = (bidx + 1) % num_mballs;
		break;

	default:
		break;
	}
}

int bnstate[32];
int prev_x, prev_y;
int mod;

void mouse(int bn, int state, int x, int y)
{
	bnstate[bn] = state == GLUT_DOWN;
	prev_x = x;
	prev_y = y;
	mod = glutGetModifiers();
}

void motion(int x, int y)
{
	int dx, dy;

	dx = x - prev_x;
	dy = y - prev_y;
	prev_x = x;
	prev_y = y;

	if(mod) {
		if(bnstate[GLUT_LEFT_BUTTON]) {
			cam_inp_rotate(dx, dy);
		}
		if(bnstate[GLUT_RIGHT_BUTTON]) {
			cam_inp_zoom(dy);
		}
	} else {
		mball[bidx].x += (float)dx * 0.005;
		mball[bidx].y -= (float)dy * 0.005;
	}
	glutPostRedisplay();
}

void sball_button(int bn, int state)
{
	if(state) return;

	if(bn < num_mballs) {
		bidx = bn;
	} else {
		bidx = (bidx + 1) % num_mballs;
	}
}

void sball_motion(int x, int y, int z)
{
	mball[bidx].x += (float)x / 16384.0;
	mball[bidx].y += (float)y / 16384.0;
	mball[bidx].z -= (float)z / 16384.0;
	glutPostRedisplay();
}

int parse_args(int argc, char **argv)
{
	int i;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-' && argv[i][2] == 0) {
			switch(argv[i][1]) {
			case 'f':
				fullscreen = !fullscreen;
				break;

			case 's':
				stereo = !stereo;
				break;

			case 'h':
				printf("usage: %s [opt]\n", argv[0]);
				printf("options:\n");
				printf("  -f    start in fullscreen\n");
				printf("  -s    enable stereoscopic rendering\n");
				printf("  -h    print usage and exit\n");
				exit(0);

			default:
				fprintf(stderr, "unrecognized option: %s\n", argv[i]);
				return -1;
			}
		} else {
			fprintf(stderr, "unexpected argument: %s\n", argv[i]);
			return -1;
		}
	}
	return 0;
}
