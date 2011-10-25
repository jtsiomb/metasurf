#include <stdlib.h>
#include "metasurf.h"
#include "mcubes.h"

#undef USE_MTETRA
#define USE_MCUBES

#if (defined(USE_MTETRA) && defined(USE_MCUBES)) || (!defined(USE_MTETRA) && !defined(USE_MCUBES))
#error "pick either USE_MTETRA or USE_MCUBES, not both..."
#endif

typedef float vec3[3];

struct metasurface {
	vec3 min, max;
	int res[3];
	float thres;

	msurf_eval_func_t eval;
	msurf_vertex_func_t vertex;
	msurf_normal_func_t normal;

	vec3 vbuf[3];
	int nverts;
};

static int msurf_init(struct metasurface *ms);
static void process_cell(struct metasurface *ms, vec3 pos, vec3 sz);
#ifdef USE_MTETRA
static void process_tetra(struct metasurface *ms, int *idx, vec3 *pos, float *val);
#endif
#ifdef USE_MCUBES
static void process_cube(struct metasurface *ms, vec3 *pos, float *val);
#endif


struct metasurface *msurf_create(void)
{
	struct metasurface *ms;

	if(!(ms = malloc(sizeof *ms))) {
		return 0;
	}
	if(msurf_init(ms) == -1) {
		free(ms);
	}
	return ms;
}

void msurf_free(struct metasurface *ms)
{
	free(ms);
}

static int msurf_init(struct metasurface *ms)
{
	ms->thres = 0.0;
	ms->eval = 0;
	ms->vertex = 0;
	ms->normal = 0;
	ms->min[0] = ms->min[1] = ms->min[2] = -1.0;
	ms->max[0] = ms->max[1] = ms->max[2] = 1.0;
	ms->res[0] = ms->res[1] = ms->res[2] = 32;
	ms->nverts = 0;

	return 0;
}

void msurf_eval_func(struct metasurface *ms, msurf_eval_func_t func)
{
	ms->eval = func;
}

void msurf_vertex_func(struct metasurface *ms, msurf_vertex_func_t func)
{
	ms->vertex = func;
}

void msurf_normal_func(struct metasurface *ms, msurf_normal_func_t func)
{
	ms->normal = func;
}

void msurf_bounds(struct metasurface *ms, float xmin, float ymin, float zmin, float xmax, float ymax, float zmax)
{
	ms->min[0] = xmin;
	ms->min[1] = ymin;
	ms->min[2] = zmin;
	ms->max[0] = xmax;
	ms->max[1] = ymax;
	ms->max[2] = zmax;
}

void msurf_resolution(struct metasurface *ms, int xres, int yres, int zres)
{
	ms->res[0] = xres;
	ms->res[1] = yres;
	ms->res[2] = zres;
}

void msurf_threshold(struct metasurface *ms, float thres)
{
	ms->thres = thres;
}


void msurf_polygonize(struct metasurface *ms)
{
	int i, j, k;
	vec3 pos, delta;

	for(i=0; i<3; i++) {
		delta[i] = (ms->max[i] - ms->min[i]) / (float)ms->res[i];
	}

	pos[0] = ms->min[0];
	for(i=0; i<ms->res[0] - 1; i++) {

		pos[1] = ms->min[1];
		for(j=0; j<ms->res[1] - 1; j++) {

			pos[2] = ms->min[2];
			for(k=0; k<ms->res[2] - 1; k++) {

				process_cell(ms, pos, delta);

				pos[2] += delta[2];
			}
			pos[1] += delta[1];
		}
		pos[0] += delta[0];
	}
}


static void process_cell(struct metasurface *ms, vec3 pos, vec3 sz)
{
	int i;
	vec3 p[8];
	float val[8];

#ifdef USE_MTETRA
	static int tetra[][4] = {
		{0, 2, 3, 7},
		{0, 2, 6, 7},
		{0, 4, 6, 7},
		{0, 6, 1, 2},
		{0, 6, 1, 4},
		{5, 6, 1, 4}
	};
#endif

	static const float offs[][3] = {
		{0.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f},
		{1.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 1.0f},
		{1.0f, 0.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{0.0f, 1.0f, 1.0f}
	};

	for(i=0; i<8; i++) {
		p[i][0] = pos[0] + sz[0] * offs[i][2];
		p[i][1] = pos[1] + sz[1] * offs[i][1];
		p[i][2] = pos[2] + sz[2] * offs[i][0];

		val[i] = ms->eval(p[i][0], p[i][1], p[i][2]);
	}

#ifdef USE_MTETRA
	for(i=0; i<6; i++) {
		process_tetra(ms, tetra[i], p, val);
	}
#endif
#ifdef USE_MCUBES
	process_cube(ms, p, val);
#endif
}


/* ---- marching cubes implementation ---- */
#ifdef USE_MCUBES

static unsigned int mc_bitcode(float *val, float thres);

static void process_cube(struct metasurface *ms, vec3 *pos, float *val)
{
	static const int pidx[12][2] = {
		{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
		{6, 7},	{7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}
	};
	int i, j;
	vec3 vert[12];
	unsigned int code = mc_bitcode(val, ms->thres);

	if(mc_edge_table[code] == 0) {
		return;
	}

	for(i=0; i<12; i++) {
		if(mc_edge_table[code] & (1 << i)) {
			int p0 = pidx[i][0];
			int p1 = pidx[i][1];

			float t = (ms->thres - val[p0]) / (val[p1] - val[p0]);
			vert[i][0] = pos[p0][0] + (pos[p1][0] - pos[p0][0]) * t;
			vert[i][1] = pos[p0][1] + (pos[p1][1] - pos[p0][1]) * t;
			vert[i][2] = pos[p0][2] + (pos[p1][2] - pos[p0][2]) * t;
		}
	}

	for(i=0; mc_tri_table[code][i] != -1; i+=3) {
		for(j=0; j<3; j++) {
			float *v = vert[mc_tri_table[code][i + j]];
			ms->vertex(v[0], v[1], v[2]);
		}
	}
}

static unsigned int mc_bitcode(float *val, float thres)
{
	unsigned int i, res = 0;

	for(i=0; i<8; i++) {
		if(val[i] > thres) {
			res |= 1 << i;
		}
	}
	return res;
}
#endif	/* USE_MCUBES */


/* ---- marching tetrahedra implementation (incomplete) ---- */
#ifdef USE_MTETRA

static unsigned int mt_bitcode(float v0, float v1, float v2, float v3, float thres);
static void emmit(struct metasurface *ms, float v0, float v1, vec3 p0, vec3 p1, int rev)


#define REVBIT(x)	((x) & 8)
#define INV(x)		(~(x) & 0xf)
#define EDGE(a, b)	emmit(ms, val[idx[a]], val[idx[b]], pos[idx[a]], pos[idx[b]], REVBIT(code))
static void process_tetra(struct metasurface *ms, int *idx, vec3 *pos, float *val)
{
	unsigned int code = mt_bitcode(val[idx[0]], val[idx[1]], val[idx[2]], val[idx[3]], ms->thres);

	switch(code) {
	/*case 1:
	case INV(1):*/
	case 0x0e:
	case 0x01:
		EDGE(0, 1);
		EDGE(0, 2);
		EDGE(0, 3);
		break;

	/*case 2:
	case INV(2):*/
	case 0x0d:
	case 0x02:
		EDGE(1, 0);
		EDGE(1, 3);
		EDGE(1, 2);
		break;

	/*case 3:
	case INV(3):*/
	case 0x0c:
	case 0x03:
		EDGE(0, 3);
		EDGE(0, 2);
		EDGE(1, 3);

		EDGE(1, 3);
		EDGE(1, 2);
		EDGE(0, 2);
		break;

	/*case 4:
	case INV(4):*/
	case 0x0b:
	case 0x04:
		EDGE(2, 0);
		EDGE(2, 1);
		EDGE(2, 3);
		break;

	/*case 5:
	case INV(5):*/
	case 0x0a:
	case 0x05:
		EDGE(0, 1);
		EDGE(2, 3);
		EDGE(0, 3);

		EDGE(0, 1);
		EDGE(1, 2);
		EDGE(2, 3);
		break;

	/*case 6:
	case INV(6):*/
	case 0x09:
	case 0x06:
		EDGE(0, 1);
		EDGE(1, 3);
		EDGE(2, 3);

		EDGE(0, 1);
		EDGE(0, 2);
		EDGE(2, 3);
		break;

	/*case 7:
	case INV(7):*/
	case 0x07:
	case 0x08:
		EDGE(3, 0);
		EDGE(3, 2);
		EDGE(3, 1);
		break;

	default:
		break;	/* cases 0 and 15 */
	}
}

#define BIT(i)	((v##i > thres) ? (1 << i) : 0)
static unsigned int mt_bitcode(float v0, float v1, float v2, float v3, float thres)
{
	return BIT(0) | BIT(1) | BIT(2) | BIT(3);
}

static void emmit(struct metasurface *ms, float v0, float v1, vec3 p0, vec3 p1, int rev)
{
	int i;
	float t = (ms->thres - v0) / (v1 - v0);

	vec3 p;
	for(i=0; i<3; i++) {
		p[i] = p0[i] + (p1[i] - p0[i]) * t;
	}
	ms->vertex(p[0], p[1], p[2]);

	/*for(i=0; i<3; i++) {
		ms->vbuf[ms->nverts][i] = p0[i] + (p1[i] - p0[i]) * t;
	}

	if(++ms->nverts >= 3) {
		ms->nverts = 0;

		for(i=0; i<3; i++) {
			int idx = rev ? (2 - i) : i;
			ms->vertex(ms->vbuf[idx][0], ms->vbuf[idx][1], ms->vbuf[idx][2]);
		}
	}*/
}

#endif	/* USE_MTETRA */
