
#ifndef POSTSCRIPT_IN
#define POSTSCRIPT_IN

typedef struct {
		float	r;
		float	g;
		float	b;
	} RGBCOLOR;

/* page size constants */
/* these were derived from portrait A4 paper on an HP Laserjet 4 */
/* Origin is bottom left */
#define PS_X_DIM		595.0	/* ~8.3 inches 72 dpi */
#define PS_Y_DIM		842.0	/* ~11.7 inches at 72 dpi */
#define PS_X_MIN		24.0	/* approximate printable area coordinate */
#define PS_Y_MIN		23.0	/* approximate printable area coordinate */
#define PS_X_MAX		571.0	/* approximate printable area coordinate */
#define PS_Y_MAX		780.0	/* approximate printable area coordinate */
#define	PS_X_CENT		(PS_X_MIN + (PS_X_MAX - PS_X_MIN) / 2)
#define	PS_Y_CENT		(PS_Y_MIN + (PS_Y_MAX - PS_Y_MIN) / 2)
#define PS_X_OFFSET		(PS_X_DIM / 2.2)
#define N_MAP_COLORS    9
#define N_COLORS        9
#define	MAX_PICTURE_RADIUS_PIX (PS_X_DIM / 4.0)

#define WHITE           0
#define YELLOW          1
#define ORANGE          2
#define RED             3
#define GREEN           4
#define BLUE            5
#define PURPLE          6
#define GRAY            7
#define BLACK           8

#define DEFAULT_POINTS_PER_PIXEL 1

#endif

