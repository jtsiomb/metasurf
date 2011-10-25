#include <math.h>
#include "cam.h"

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#define DR				(M_PI / 180.0)
#define DEG_TO_RAD(x)	((x) * DR)

typedef struct vec3 {
	float x, y, z;
} vec3_t;

/* viewing parameters */
#define DEF_THETA	0
#define DEF_PHI		0
#define DEF_DIST	0
#define DEF_X		0
#define DEF_Y		0
#define DEF_Z		0

static float cam_theta = DEF_THETA, cam_phi = DEF_PHI;
static float cam_dist = DEF_DIST;
static vec3_t cam_pos = {DEF_X, DEF_Y, DEF_Z};

/* projection parameters */
#define DEF_VFOV	45.0
#define DEF_ASPECT	1.3333333
#define DEF_NEAR	1.0
#define DEF_FAR		1000.0

static float vfov = DEF_VFOV;
static float aspect = DEF_ASPECT;
static float nearclip = DEF_NEAR, farclip = DEF_FAR;

/* stereo parameters */
#define DEF_EYE_SEP		0.1
#define DEF_FOCUS_DIST	1.0

static float eye_sep = DEF_EYE_SEP;
static float focus_dist = DEF_FOCUS_DIST;

static float pan_speed = 0.001;
static float rot_speed = 0.5;
static float zoom_speed = 0.1;

static float vmin_deg = -90.0, vmax_deg = 90.0;

void cam_reset(void)
{
	cam_reset_view();
	cam_reset_proj();
	cam_reset_stereo();
}

void cam_reset_view(void)
{
	cam_theta = DEF_THETA;
	cam_phi = DEF_PHI;
	cam_dist = DEF_DIST;
	cam_pos.x = DEF_X;
	cam_pos.y = DEF_Y;
	cam_pos.z = DEF_Z;
}

void cam_reset_proj(void)
{
	vfov = DEF_VFOV;
	aspect = DEF_ASPECT;
	nearclip = DEF_NEAR;
	farclip = DEF_FAR;
}

void cam_reset_stereo(void)
{
	eye_sep = DEF_EYE_SEP;
	focus_dist = DEF_FOCUS_DIST;
}

void cam_set_vrange(float min_deg, float max_deg)
{
	vmin_deg = min_deg;
	vmax_deg = max_deg;
}

void cam_move(float x, float y, float z)
{
	cam_pos.x += x;
	cam_pos.y += y;
	cam_pos.z += z;
}

void cam_rotate(float theta, float phi)
{
	cam_phi += phi;
	cam_theta += theta;
}

void cam_dolly(float dist)
{
	cam_dist += dist;
}

void cam_inp_pan_speed(float speed)
{
	pan_speed = speed;
}

void cam_inp_rotate_speed(float speed)
{
	rot_speed = speed;
}

void cam_inp_zoom_speed(float speed)
{
	zoom_speed = speed;
}


void cam_inp_pan(int dx, int dy)
{
	float dxf = dx * pan_speed;
	float dyf = dy * pan_speed;
	float angle = -DEG_TO_RAD(cam_theta);

	cam_pos.x += cos(angle) * dxf + sin(angle) * dyf;
	cam_pos.z += -sin(angle) * dxf + cos(angle) * dyf;
}

void cam_inp_height(int dh)
{
	cam_pos.y += dh * pan_speed;
}

void cam_inp_rotate(int dx, int dy)
{
	cam_theta += dx * rot_speed;
	cam_phi += dy * rot_speed;

	if(cam_phi < vmin_deg) cam_phi = vmin_deg;
	if(cam_phi > vmax_deg) cam_phi = vmax_deg;
}

void cam_inp_zoom(int dz)
{
	cam_dist += dz * zoom_speed;
	if(cam_dist < 0.001) {
		cam_dist = 0.001;
	}
}

void cam_clip(float n, float f)
{
	nearclip = n;
	farclip = f;
}

void cam_fov(float f)
{
	vfov = f;
}

void cam_aspect(float a)
{
	aspect = a;
}

void cam_separation(float s)
{
	eye_sep = s;
}

void cam_focus_dist(float d)
{
	focus_dist = d;

	cam_separation(d / 30.0);
}

void cam_view_matrix(void)
{
	cam_stereo_view_matrix(CAM_CENTER);
}

void cam_stereo_view_matrix(int eye)
{
	static const float offs_sign[] = {0.0f, 0.5f, -0.5f};	/* center, left, right */
	float offs = eye_sep * offs_sign[eye];

	glTranslatef(offs, 0, 0);

	glTranslatef(0, 0, -cam_dist);
	glRotatef(cam_phi, 1, 0, 0);
	glRotatef(cam_theta, 0, 1, 0);
	glTranslatef(-cam_pos.x, -cam_pos.y, -cam_pos.z);
}

void cam_proj_matrix(void)
{
	cam_stereo_proj_matrix(CAM_CENTER);
}

void cam_stereo_proj_matrix(int eye)
{
	float vfov_rad = M_PI * vfov / 180.0;
	float top = nearclip * tan(vfov_rad * 0.5);
	float right = top * aspect;

	static const float offs_sign[] = {0.0f, 1.0, -1.0};	/* center, left, right */
	float frust_shift = offs_sign[eye] * (eye_sep * 0.5 * nearclip / focus_dist);

	glFrustum(-right + frust_shift, right + frust_shift, -top, top, nearclip, farclip);
}
