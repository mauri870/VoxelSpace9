#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <draw.h>
#include <event.h>

/* Window dimensions */
Point windowDimensions;

/* Menus */
char *buttons[] = {"exit", 0};
Menu menu = { buttons };

void VoxelSpace_init(void);
void VoxelSpace_draw(void);
void VoxelSpace_render(void);

void
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0)
		sysfatal("can't reattach to window");

	/* Store new screen coordinates */
	windowDimensions = Pt(Dx(screen->r), Dy(screen->r));

	/* Draw the background Red */
	draw(screen, insetrect(screen->r, 20), 
			allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DRed), 
			nil, ZP);
}


void
main(int argc, char *argv[])
{
	USED(argc, argv);

	Event ev;
	int e, timer;

	/* Initiate graphics and mouse */
	if(initdraw(nil, nil, "Voxel Space") < 0)
		sysfatal("initdraw failed: %r");

	einit(Emouse);

    /* Timer for the event loop */
	timer = etimer(0, 200);

    /* Trigger a initial resize to paint initial color on screen */
	eresized(0);

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
            fprintf(2, "Tick...");
	}
}
