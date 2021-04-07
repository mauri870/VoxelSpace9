#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <draw.h>
#include <event.h>
#include <memdraw.h>

/* Colormap image file */
Memimage *cmapim;
/* Heightmap image file */
Memimage *hmapim;

int screenwidth = 400;
int screenheight = 300;

int px = 800;
int py = 500;
double pd = 1.7;
int bgColor = (144 << 24) + (144 << 16) + (224 << 8);
Image *bgim;

/* Menus */
char *buttons[] = {"exit", 0};
Menu menu = {buttons};

int getColorFromImage(Memimage *im, Point p) {
	uchar data[3];
	int color;

	int ret = unloadmemimage(im, Rect(p.x, p.y, p.x + 1, p.y + 1), data,
				 sizeof data);
	if (ret < 0) return -1;

	/* Height map has chan k8 */
	if (ret == 1) return data[0];

	/* Color map image has chan r8g8b8 but data is actually b g r */
	color |= (data[2] & 255) << 24;
	color |= (data[1] & 255) << 16;
	color |= (data[0] & 255) << 8;

	return color;
}

void paintRgb(Memimage *frame, int x, int y, int color) {
	Memimage *mi;
	Rectangle r;

	mi = allocmemimage(Rect(0, 0, 1, 1), frame->chan);
	memfillcolor(mi, color);

	r = frame->r;
	r.min.x += x;
	r.min.y += y;
	r.max.x = r.min.x + 1;
	r.max.y = r.min.y + 1;

	memimagedraw(frame, r, mi, ZP, nil, ZP, S);
	freememimage(mi);
}

void writePixel(Image *dst, Point p, int color) {
	Rectangle r;

	r = dst->r;
	r.min = addpt(r.min, p);
	r.max = addpt(r.min, Pt(1, 1));

	// FIXME: This assumes color to be 24 bit RGB
	uchar bits[4];
	bits[2] = color >> 24;
	bits[1] = color >> 16;
	bits[0] = color >> 8;
	loadimage(dst, r, bits, sizeof bits);
}

void drawVerticalLine(Image *dst, int x, int ytop, int ybottom, int color) {
	Rectangle r;

	if (ytop < 0) ytop = 0;
	if (ytop > ybottom) return;

	r = dst->r;
	r.min.x += x;
	r.min.y += (ybottom - ytop);
	r.max.x = r.min.x + 1;
	r.max.y = ybottom;

	// FIXME: This assumes color to be 24 bit RGB
	uchar bits[4];
	bits[2] = color >> 24;
	bits[1] = color >> 16;
	bits[0] = color >> 8;
	loadimage(dst, r, bits, sizeof bits);
}

int addFog(int color, int depth) {
	int r = (color >> 24) & 255;
	int g = (color >> 16) & 255;
	int b = (color >> 8) & 255;
	double p = 0.0;
	if (depth > 100) p = (depth - 100) / 500.0;
	r = (int)(r + (150 - r) * p);
	g = (int)(g + (170 - g) * p);
	b = (int)(b + (170 - b) * p);
	return (r << 24) + (g << 16) + (b << 8);
}

void render(void) {
	int mapwidthperiod = 1023;
	int mapheightperiod = 1023;
	int *hiddeny;

	int screenwidth = Dx(screen->r);
	int screenheight = Dy(screen->r);
	int distance = 300;
	int shift = 10;
	int horizon = 100;
	int height = 78;

	double sinang = sin(pd);
	double cosang = cos(pd);

	hiddeny = malloc(screenwidth * sizeof(int));
	for (int i = 0; i < screenwidth; i++)
		hiddeny[i] = screenheight;
	
	double deltaz = 1.0;

	// Draw from front to back
    for(int z = 1; z < distance; z+=deltaz)
    {
        // 90 degree field of view
        int plx =  -cosang * z - sinang * z;
        int ply =  sinang * z - cosang * z;
        int prx =  cosang * z - sinang * z;
        int pry =  -sinang * z - cosang * z;

        double dx = (prx - plx) / screenwidth;
        double dy = (pry - ply) / screenwidth;
        plx += px;
        ply += py;
        double invz = 1. / z * 240.;
        for(int i=0; i<screenwidth; i++)
        {
            // int mapoffset = ((floor(ply) & mapwidthperiod) << shift) + (floor(plx) & mapheightperiod);
			int alt = getColorFromImage(hmapim, Pt(plx, ply)) & 255;
			// TODO: not sure about this
			int color = addFog(getColorFromImage(cmapim, Pt(plx, ply)), height);
            int heightonscreen = (height - alt) * invz + horizon;
            drawVerticalLine(screen, i, heightonscreen, hiddeny[i], color);
            if (heightonscreen < hiddeny[i]) hiddeny[i] = heightonscreen;
            plx += dx;
            ply += dy;
        }
        deltaz += 0.005;
    }

	free(hiddeny);
}

void drawBackground(void) {
	draw(screen, screen->r, bgim, nil, ZP);
}

void redraw(void) {
	drawBackground();
	render();

	px += 2 * cos(pd);
	py += 2 * sin(pd);
	pd += 0.01;
}

void eresized(int new) {
	if (new && getwindow(display, Refnone) < 0)
		sysfatal("can't reattach to window");

	redraw();
}

int loadImage(char *file, Memimage **i) {
	int fd;
	if ((fd = open(file, OREAD)) < 0) return -1;

	if ((*i = readmemimage(fd)) == nil) return -1;

	close(fd);
	return 0;
}

void drawString(char *msg) {
	string(screen, Pt(100, 100),
	       allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DRed), ZP,
	       font, msg);
}

void _images_cleanup(void) {
	freememimage(cmapim);
	freememimage(hmapim);
}

void main(int argc, char *argv[]) {
	Event ev;
	int e, timer;

	if (argc != 3) {
		sysfatal("Please provide colormap and heightmap file");
	}

	memimageinit();

	/* Initiate graphics and mouse */
	if (initdraw(nil, nil, "Voxel Space") < 0)
		sysfatal("initdraw failed: %r");

	/* Load cmap and hmap images */
	if (loadImage(argv[1], &cmapim) < 0) sysfatal("LoadImage cmap: %r");
	if (loadImage(argv[2], &hmapim) < 0) sysfatal("LoadImage hmap: %r");

	/* Allocate background color only once */
	bgim = allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, bgColor);

	/* cleanup images */
	atexit(_images_cleanup);

	/* Trigger a initial resize */
	eresized(0);

	einit(Emouse);

	/* Timer for the event loop */
	timer = etimer(0, 200);

	/* Main event loop */
	for (;;) {
		e = event(&ev);

		/* Check if user select the exit option from menu context */
		if ((e == Emouse) && (ev.mouse.buttons & 4) &&
		    (emenuhit(3, &ev.mouse, &menu) == 0))
			exits(nil);

		/* If the timer ticks... */
		if (e == timer) redraw();
	}
}
