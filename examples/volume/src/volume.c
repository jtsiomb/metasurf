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

#include <imago2.h>

#include "cam.h"
#include "metasurf.h"

float eval(float x, float y, float z);
void vertex(float x, float y, float z);
void render(void);
void disp(void);
void reshape(int x, int y);
void keyb(unsigned char key, int x, int y);
void keyb_up(unsigned char key, int x, int y);
void mouse(int bn, int state, int x, int y);
void motion(int x, int y);
int parse_args(int argc, char **argv);

int stereo, fullscreen;
int orig_xsz, orig_ysz;

struct metasurface *msurf;
float threshold = 0.5;
#ifndef NO_SHADERS
unsigned int sdr;
#endif

float yscale = 1.0;

struct img_pixmap *volume;
int xres, yres, num_slices;

int dlist, need_update = 1;

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
	glutCreateWindow("metasurf - volume rendering");

	orig_xsz = glutGet(GLUT_WINDOW_WIDTH);
	orig_ysz = glutGet(GLUT_WINDOW_HEIGHT);

	if(fullscreen) {
		glutFullScreen();
	}

	glutDisplayFunc(disp);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyb);
	glutKeyboardUpFunc(keyb_up);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

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

	cam_focus_dist(3.0);
	cam_clip(0.1, 200.0);
	cam_rotate(0, 0);
	cam_dolly(2);

	msurf = msurf_create();
	msurf_eval_func(msurf, eval);
	msurf_vertex_func(msurf, vertex);
	msurf_threshold(msurf, threshold);
	msurf_resolution(msurf, xres, yres, num_slices);
	msurf_bounds(msurf, -1, -1, -1, 1, 1, 1);

	glClearColor(0.6, 0.6, 0.6, 1.0);

	dlist = glGenLists(1);

	glutMainLoop();
	return 0;
}

float eval(float x, float y, float z)
{
	int px, py, slice;
	struct img_pixmap *img;

	px = round((x * 0.5 + 0.5) * xres);
	py = round((y * 0.5 + 0.5) * yres);
	slice = round((z * 0.5 + 0.5) * num_slices);

	if(px < 0) px = 0;
	if(px >= xres) px = xres - 1;

	if(py < 0) py = 0;
	if(py >= yres) py = yres - 1;

	if(slice < 0) slice = 0;
	if(slice >= num_slices) slice = num_slices - 1;

	img = volume + slice;
	return *((unsigned char*)img->pixels + py * img->width + px) / 255.0;
}

void vertex(float x, float y, float z)
{
	float dx = 1.0 / xres;
	float dy = 1.0 / yres;
	float dz = 1.0 / num_slices;
	float dfdx = eval(x - dx, y, z) - eval(x + dx, y, z);
	float dfdy = eval(x, y - dy, z) - eval(x, y + dy, z);
	float dfdz = eval(x, y, z - dz) - eval(x, y, z + dz);

	glNormal3f(dfdx, dfdy, dfdz);
	glVertex3f(x, y, z);
}

void render(void)
{
	float kd[] = {0.87, 0.82, 0.74, 1.0};
	float ks[] = {0.9, 0.9, 0.9, 1.0};

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, kd);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ks);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 60.0);

#ifndef NO_SHADERS
	bind_program(sdr);
#endif

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glScalef(1.0, yscale, 1.0);
	glRotatef(90, 1, 0, 0);

	if(need_update) {
		glNewList(dlist, GL_COMPILE);
		glBegin(GL_TRIANGLES);
		printf("generating mesh... ");
		fflush(stdout);
		msurf_polygonize(msurf);
		glEnd();
		glEndList();
		need_update = 0;
		printf("done\n");
	}
	glCallList(dlist);

	glPopMatrix();

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

int mode_scale;

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
		msurf_threshold(msurf, threshold);
		printf("threshold: %f\n", threshold);
		glutPostRedisplay();
		need_update = 1;
		break;

	case '-':
		threshold -= 0.05;
		msurf_threshold(msurf, threshold);
		printf("threshold: %f\n", threshold);
		glutPostRedisplay();
		need_update = 1;
		break;

	case 'y':
		mode_scale = 1;
		break;

	default:
		break;
	}
}

void keyb_up(unsigned char key, int x, int y)
{
	switch(key) {
	case 'y':
		mode_scale = 0;
		break;
	}
}

int bnstate[32];
int prev_x, prev_y;

void mouse(int bn, int state, int x, int y)
{
	bnstate[bn] = state == GLUT_DOWN;
	prev_x = x;
	prev_y = y;
}

void motion(int x, int y)
{
	int dx, dy;

	dx = x - prev_x;
	dy = y - prev_y;
	prev_x = x;
	prev_y = y;

	if(mode_scale) {
		yscale += dy * 0.001;
	} else {
		if(bnstate[GLUT_LEFT_BUTTON]) {
			cam_inp_rotate(dx, dy);
		}
		if(bnstate[GLUT_RIGHT_BUTTON]) {
			cam_inp_zoom(dy);
		}
	}
	glutPostRedisplay();
}

struct list_node {
	struct img_pixmap img;
	struct list_node *next;
};

int parse_args(int argc, char **argv)
{
	int i;
	char *endp;
	struct list_node *head = 0, *tail = 0;

	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-' && argv[i][2] == 0) {
			switch(argv[i][1]) {
			case 'f':
				fullscreen = !fullscreen;
				break;

			case 's':
				stereo = !stereo;
				break;

			case 't':
				threshold = strtod(argv[++i], &endp);
				if(endp == argv[i]) {
					fprintf(stderr, "-t must be followed by a number\n");
					return -1;
				}
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
			struct list_node *slice;

			if(!(slice = malloc(sizeof *slice))) {
				fprintf(stderr, "failed to allocate volume slice: %d\n", num_slices);
				return -1;
			}
			slice->next = 0;

			img_init(&slice->img);
			if(img_load(&slice->img, argv[i]) == -1) {
				fprintf(stderr, "failed to load volume slice %d: %s\n", num_slices, argv[i]);
				free(slice);
				return -1;
			}
			img_convert(&slice->img, IMG_FMT_GREY8);

			if(num_slices > 0 && (xres != slice->img.width || yres != slice->img.height)) {
				fprintf(stderr, "error: slice %d (%s) is %dx%d, up to now we had %dx%d images\n", num_slices, argv[i],
						slice->img.width, slice->img.height, xres, yres);
				img_destroy(&slice->img);
				free(slice);
				return -1;
			}
			xres = slice->img.width;
			yres = slice->img.height;

			if(head) {
				tail->next = slice;
				tail = slice;
			} else {
				head = tail = slice;
			}
			printf("loaded volume slice %d: %s\n", num_slices++, argv[i]);
		}
	}

	if(!head) {
		fprintf(stderr, "you must specify a list of images for the volume data slices\n");
		return -1;
	}

	if(!(volume = malloc(num_slices * sizeof *volume))) {
		fprintf(stderr, "failed to allocate volume data (%d slices)\n", num_slices);
		return -1;
	}

	for(i=0; i<num_slices; i++) {
		void *tmp;

		assert(head);
		volume[i] = head->img;

		tmp = head;
		head = head->next;
		free(tmp);
	}

	return 0;
}
