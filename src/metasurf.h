#ifndef METASURF_H_
#define METASURF_H_

struct metasurface;

typedef float (*msurf_eval_func_t)(float, float, float);
typedef void (*msurf_vertex_func_t)(float, float, float);
typedef void (*msurf_normal_func_t)(float, float, float);

#ifdef __cplusplus
extern "C" {
#endif

struct metasurface *msurf_create(void);
void msurf_free(struct metasurface *ms);

void msurf_eval_func(struct metasurface *ms, msurf_eval_func_t func);
void msurf_vertex_func(struct metasurface *ms, msurf_vertex_func_t func);
void msurf_normal_func(struct metasurface *ms, msurf_normal_func_t func);

void msurf_bounds(struct metasurface *ms, float xmin, float ymin, float zmin, float xmax, float ymax, float zmax);
void msurf_resolution(struct metasurface *ms, int xres, int yres, int zres);
void msurf_threshold(struct metasurface *ms, float thres);

void msurf_polygonize(struct metasurface *ms);

#ifdef __cplusplus
}
#endif

#endif	/* METASURF_H_ */
