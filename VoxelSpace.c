#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <draw.h>
#include <event.h>

/* Window dimensions */
Point windowDimensions;

/* Colormap image file */
Image *cmapim;

/* Menus */
char *buttons[] = {"exit", 0};
Menu menu = { buttons };

void
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0)
		sysfatal("can't reattach to window");

	/* Store new screen coordinates */
	windowDimensions = Pt(Dx(screen->r), Dy(screen->r));

	/* Draw the background White */
	draw(screen, insetrect(screen->r, 20), 
			allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DWhite), nil, ZP);
}

void
main(int argc, char *argv[])
{
	int fd;
	Event ev;
	int e, timer;

	if (argc != 2) {
		sysfatal("Please provide colormap file");
	}

	/* Open colormap image file */
	if((fd = open(argv[1], OREAD)) < 0)
		sysfatal("open %s: %r", argv[1]);

	/* Initiate graphics and mouse */
	if(initdraw(nil, nil, "Voxel Space") < 0)
		sysfatal("initdraw failed: %r");

	/* Trigger a initial resize to paint initial color on screen */
	eresized(0);

	/* Read colormap into an Image */
	if((cmapim = readimage(display, fd, 0)) == nil)
		sysfatal("readimage: %r");
	close(fd);

	/* Draw image on screen */
	draw(screen, screen->r, cmapim, nil, ZP);

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
            fprint(2, "Tick...");
	}
}
