#ifndef CAM_H_
#define CAM_H_

enum {
	CAM_CENTER,
	CAM_LEFT,
	CAM_RIGHT
};

/* reset to the initial state */
void cam_reset(void);			/* all */
void cam_reset_view(void);		/* view parameters */
void cam_reset_proj(void);		/* projection parameters */
void cam_reset_stereo(void);	/* stereo parameters */

void cam_set_vrange(float min_deg, float max_deg);

void cam_move(float x, float y, float z);
void cam_rotate(float theta, float phi);
void cam_dolly(float dist);

/* camera input handling */
void cam_inp_pan_speed(float speed);
void cam_inp_rotate_speed(float speed);
void cam_inp_zoom_speed(float speed);

void cam_inp_pan(int dx, int dy);		/* pan across X/Z plane */
void cam_inp_height(int dh);			/* move verticaly */
void cam_inp_rotate(int dx, int dy);	/* rotate around local Y and X axis */
void cam_inp_zoom(int dz);				/* dolly the camera fwd/back */

/* camera projection parameters */
void cam_clip(float n, float f);	/* set clipping planes */
void cam_fov(float f);				/* vertical field of view in degrees */
void cam_aspect(float a);			/* aspect ratio (width / height) */

/* stereo parameters */
void cam_separation(float s);
void cam_focus_dist(float d);


/* multiply the camera view matrix on top of the current matrix stack
 * (which should be GL_MODELVIEW)
 */
void cam_view_matrix(void);
void cam_stereo_view_matrix(int eye);

/* multiply the camera projection matrix on top of the current matrix stack
 * (which should be GL_PROJECTION)
 */
void cam_proj_matrix(void);
void cam_stereo_proj_matrix(int eye);

#endif	/* CAM_H_ */
