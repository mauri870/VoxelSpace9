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
int backgroundColor;

/* Menus */
char *buttons[] = {"exit", 0};
Menu menu = {buttons};

int getColorFromImage(Memimage *im, Point p) {
	uchar data[1];
	int ret = unloadmemimage(im, Rect(p.x, p.y, p.x + 1, p.y + 1), data,
			      sizeof data);
	if (ret < 0) return -1;
	return cmap2rgb(data[0]);
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

void drawVerticalLine(Memimage *frame, int x, int ytop, int ybottom, int color) {
	Memimage *mi;
	Rectangle r;

	if (ytop < 0) ytop = 0;
	if (ytop > ybottom) return;

	mi = allocmemimage(Rect(0, 0, 1, 1), frame->chan);
	memfillcolor(mi, color);

	r = frame->r;
	r.min.x += x;
	r.min.y += (ybottom - ytop);
	r.max.x = r.min.x + 1;
	r.max.y = ybottom;

	memimagedraw(frame, r, mi, ZP, nil, ZP, S);
	freememimage(mi);
}

void render(void) {
	Memimage *frame;
	uchar *bits;
	int nbits;

	frame = allocmemimage(screen->r, screen->chan);
	memfillcolor(frame, backgroundColor);

	int sx = 0;
	for (double angle = -0.5; angle < 1; angle += 0.0035) {
		int maxScreenHeight = screenheight;
		double s = cos(pd + angle);
		double c = sin(pd + angle);

		for (int depth = 10; depth < 600; depth += 1) {
			int hmx = (int)(px + depth * s);
			int hmy = (int)(py + depth * c);
			int mapWidth = Dx(cmapim->r);
			if (hmx < 0 || hmy < 0 || hmx > mapWidth ||
			    hmy > mapWidth)
				continue;

			int height =
			    getColorFromImage(hmapim, Pt(hmx, hmy)) & 255;
			int color = getColorFromImage(cmapim, Pt(hmx, hmy));

			double sy = 120 * (300 - height) / depth;
			if (sy > maxScreenHeight) continue;

			for (int y = (int)sy; y <= maxScreenHeight; y++) {
				if (y < 0 || sx > screenwidth ||
				    y > screenheight)
					continue;
				paintRgb(frame, sx, y, color);
			}
			maxScreenHeight = (int)sy;
		}
		sx++;
	}

	nbits = bytesperline(frame->r, frame->depth) * Dy(frame->r);
	bits = malloc(nbits);
	unloadmemimage(frame, frame->r, bits, nbits);
	loadimage(screen, screen->r, bits, nbits);

	free(bits);
	freememimage(frame);
}

void redraw(void) {
	render();

	px += 2 * cos(pd);
	py += 2 * sin(pd);
	pd += 0.01;
}

void eresized(int new) {
	if (new &&getwindow(display, Refnone) < 0)
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

	backgroundColor = cmap2rgb(170);

	if (argc != 3) {
		sysfatal("Please provide colormap and heightmap file");
	}

	/* Initiate graphics and mouse */
	if (initdraw(nil, nil, "Voxel Space") < 0)
		sysfatal("initdraw failed: %r");

	/* Load cmap and hmap images */
	if (loadImage(argv[1], &cmapim) < 0) sysfatal("LoadImage cmap: %r");
	if (loadImage(argv[2], &hmapim) < 0) sysfatal("LoadImage hmap: %r");

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
