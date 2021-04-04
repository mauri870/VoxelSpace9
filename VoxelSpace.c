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
redraw(void) {
	draw(screen, screen->r, cmapim, nil, ZP);
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

	/* Store new screen coordinates */
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

	/* Trigger a initial resize to paint initial color on screen */
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
			fprint(2, "Tick...");
	}
}
