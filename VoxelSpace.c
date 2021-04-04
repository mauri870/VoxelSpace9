#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <draw.h>
#include <event.h>

/* Window dimensions */
Point windowDimensions;

/* Colormap image file */
Image *cmapim;
/* Heightmap image file */
Image *hmapim;

/* Menus */
char *buttons[] = {"exit", 0};
Menu menu = { buttons };

void 
render(void) {
	int height = 50;
	int angle = 0;
	int horizon = 120;
	int scale_height = 120;
	int distance = 300;
	int screenwidth = windowDimensions.x;
	int screenheight = windowDimensions.y;
	int heightOnScreen;

	double c, s, dz, z;
	int ybuf[1024];

	Point pleft, pright;
	Point p = ZP;

	s = sin(angle);
	c = cos(angle);

	for (int i = 0; i <= screenwidth; i++)
		ybuf[i] = screenheight;

	dz = 1.0;
	z = 1.0;

	while (z < (double) distance) {
		pleft = Pt((-c * z - s * z) + p.x, ( s * z - c * z) + p.y);
		pright = Pt((c * z - s * z) + p.x, ( -s * z - c * z) + p.y);

		dx = (addpt(pright, pleft)).x / screenwidth;
		dy = (subpt(pright, pleft)).x / screenwidth;

		for (int i = 0; i <= screenwidth; i++) {
			heightOnScreen = (int)((height - nrand(0, 255)) / z * scale_height + horizon);
			// TODO: Draw vertical line
			// draw(screen, screen->r, cmapim, nil, ZP);
			if (heightOnScreen < ybuf[i])
				ybuf[i] = heightOnScreen;
			
			pleft = addpt(pleft, Pt((int)dx, (int)dy))
		}

		z += dx;
		dz += 0.2;
	}
}

void 
redraw(void) {
	render();
}

void 
setBackgroundColor(ulong color) {
	draw(screen, screen->r, 
		allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, color), nil, ZP);
}

void
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0)
		sysfatal("can't reattach to window");

	/* Store new screen dimensions */
	windowDimensions = Pt(Dx(screen->r), Dy(screen->r));

	redraw();
}

int 
loadImage(char *file, Image **i) {
	int fd;
	if((fd = open(file, OREAD)) < 0)
		return -1;

	if((*i = readimage(display, fd, 0)) == nil)
		return -1;

	close(fd);
	return 0;
}

void
drawString(char *msg) {
	string(screen, Pt(100, 100), allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DRed), ZP, font, msg);
}

void
main(int argc, char *argv[])
{
	Event ev;
	int e, timer;

	if (argc != 3) {
		sysfatal("Please provide colormap and heightmap file");
	}

	/* Initiate graphics and mouse */
	if(initdraw(nil, nil, "Voxel Space") < 0)
		sysfatal("initdraw failed: %r");
	
	/* Load cmap and hmap images */
	if (loadImage(argv[1], &cmapim) < 0)
		sysfatal("LoadImage cmap: %r");
	if (loadImage(argv[2], &hmapim) < 0)
		sysfatal("LoadImage hmap: %r");

	/* Trigger a initial resize */
	eresized(0);

	/* Paint initial color on screen */
	setBackgroundColor(DWhite);

	einit(Emouse);

    /* Timer for the event loop */
	timer = etimer(0, 200);

    /* TODO: initialize everything here */

	/* Main event loop */
	for(;;)
	{
		e = event(&ev);

		/* Check if user select the exit option from menu context */
		if( (e == Emouse) &&
			(ev.mouse.buttons & 4) && 
			(emenuhit(3, &ev.mouse, &menu) == 0)) exits(nil);

        /* If the timer ticks... */
        if(e == timer)
			redraw();
	}
}
