#include "mb.h"

#define NULL 0
#define rgb(r,g,b) (r<<16)|(g<<8)|b

typedef struct {
    short xres;
    short yres;
    short videomode;
    long  videobanksize;
    short (*bankswitch)(short);
    short screenwidth;
    short screenheight;
} wgt_sys;
extern wgt_sys WGT_SYS;

/* A single palette register definition */
typedef struct
{
    unsigned char r,g,b;
} color;
extern unsigned int vgapal[256];

/* A font definition */
typedef unsigned char * wgtfont;
extern unsigned char vgafont[224][8];

/* Text grid settings */
#define TEXTGRID_OFF 0
#define TEXTGRID_ON 1

/* Text transparency settings */
#define TEXTFG 0
#define TEXTBG 1
#define TEXTFGBG 2

/* Image transfer operations */
#define NORMAL 0
#define XRAY 1

/* Pointer to the active drawing page */
typedef unsigned int *block;
extern block abuf;

/* Pointer to the hardware framebuffer */
extern block fbuf;

/* Current drawing color */
extern unsigned int currentcolor;

/* Clipping boundaries */
extern short bx,by,tx,ty;

/* Polygon point definition */
typedef struct
{
  short x,y;   /* Coordinate on the screen */
  short sx,sy; /* Coordinate on the texture if texture is used,
                  otherwise sx = color point */
               /* sx is used for Gouraud shading as well (color of the point) */
} tpolypoint;

/* whollowpoly types */
#define CLOSED_POLY 0
#define OPEN_POLY 1

/* Mouse variables */
extern volatile int but;
extern volatile int mx;
extern volatile int my;

/* Bluetooth control */
extern volatile unsigned int comms_up;
void comms_core(void);

// ######## HELPER FUNCTIONS ########

void *memset(void *dest, unsigned int val, unsigned len);
void *memcpy(void *dest, const void *src, unsigned len);
void *memcpy_xray(void *dest, const void *src, unsigned len);
void *memcpy_char(void *dest, const void *src, unsigned len);
int memcmp(char *str1, char *str2, unsigned count);
int abs(int i);
int strlen(const char *str);

// ######## WGT FUNCTIONS ########

void debugstr(char *str);
void debugcrlf(void);
void debugreset(void);
void debugch(unsigned char b);
void debughex(unsigned int d);

void delay(unsigned int n);
void wait_msec(unsigned int n);

void vga256(void);
void wsetcolor (unsigned int col);
void wline (short x, short y, short x2, short y2);
void whline (short x1, short x2, short y);
void wcls (unsigned int col);
unsigned int wgetpixel (short x, short y);
void wputpixel (short x, short y);
void wfastputpixel (short x, short y);
void wclip (short x, short y, short x2, short y2);
void wcircle (short x_center, short y_center, short radius);
void wfill_circle (short x_center, short y_center, short radius);
void wrectangle (short x, short y, short x2, short y2);
void wbar (short x, short y, short x2, short y2);
void wbutt (short x, short y, short x2, short y2);
void wsetrgb (unsigned char num, unsigned char red, unsigned char green, unsigned char blue, color *pal);
void wsetpalette (unsigned char start, unsigned char finish, color *pal);
void wreadpalette (unsigned char start, unsigned char finish, color *palc);
void wfline (short x, short y, short x2, short y2);
void wstyleline (short x, short y, short x2, short y2, unsigned short style);
void wellipse (short x_center, short y_center, short x_radius, short y_radius);
void wfill_ellipse (short x_center, short y_center, short x_radius, short y_radius);
void wouttextxy (short x, short y, wgtfont font, char *string);
short woutchar (short asc, short xc, short yc, wgtfont font);
void wtextcolor (unsigned int col);
void wtextbackground (unsigned int col);
void wtexttransparent (short transparent);
void wtextgrid (short onoff);
void wregionfill (short x, short y);
void wfreeblock (block ptr);
short wgetblockwidth (block ptr);
short wgetblockheight (block ptr);
block wnewblock (short x, short y, short x2, short y2);
void wputblock (short x, short y, block src, short method);
block wallocblock (short width, short height);
void wdonetimer (void);
void winittimer (void);
void wsettimerspeed (int speed);
void wstarttimer (void (*rout)(), int speed);
void wstoptimer (void);
void wflipblock (block image, short direction);
block wloadblock (unsigned char *data);
void wloadpalette (unsigned char *data, color *pal);
void wresize (short x, short y, short x2, short y2, block image, short mode);
void wdissolve (block sourceimage, short *pattern, short speed);
void wsetscreen (block image);
void wnormscreen (void);
void wvertres (short x, short y, short y2, block image);
void wwipe (short x, short y, short x2, short y2, block image);
void wskew (short x, short y, block image, short degrees);
short wloadsprites (color *pal, unsigned char *file, block *image_array, short start, short end);
void wfreesprites (block *image_array, short start, short end);
void wcopyscreen (short x, short y, short x2, short y2, block source, short destx, short desty, block dest);
void wsline (short x, short y, short x2, short y2, short *y_array);
void wwarp (short sx, short ex, short *tpy, short *bty, block ptr, short mode);
void wresize_column (short x, short y, short y2, block image, short column, short mode);
void wsolidpoly (tpolypoint *vertexlist, short numvertex, short x, short y, void (*customline)(short, short, short));
void whollowpoly(tpolypoint *vertexlist, short numvertex, short x, short y, short closemode);
void wbezier (tpolypoint *rawpts, short numraw, tpolypoint *curvepts, short numcurve);
void minit();
void mdeinit();
void msetxy (short x, short y);
void msetbut (short event, short bnum);
void msetbounds (short x1, short y1, short x2, short y2);
void noclick();
