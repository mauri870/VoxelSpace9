#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <draw.h>
#include <event.h>
#include <memdraw.h>

#define WSIZE 1024
#define HSIZE 1024
/* Colormap pixel data */
int cmap[WSIZE][HSIZE];
/* Heightmap pixel data */
int hmap[WSIZE][HSIZE];

double camerax = 800;
double cameray = 800;
double cameraAngle = 0;
int cameraHorizon = 120;
int cameraDistance = 1000;
int cameraHeight = 150;

int bgColor = (150 << 24) + (170 << 16) + (170 << 8);

/* Menus */
char *buttons[] = {"exit", 0};
Menu menu = {buttons};

int getPixelColor(Memimage *im, Point p) {
	uchar data[3];
	int color;

	int ret = unloadmemimage(im, Rect(p.x, p.y, p.x + 1, p.y + 1), data,
				 sizeof data);
	if (ret < 0) return -1;

	/* Height map has chan k8 */
	if (ret == 1) return data[0];

	/* Color map image has chan r8g8b8 but data is actually b g r in memory */
	color = (data[2] & 255) << 24;
	color |= (data[1] & 255) << 16;
	color |= (data[0] & 255) << 8;

	return color;
}

void writeMemimagePixel(Memimage *frame, Point p, int color) {
	Rectangle r;

	r = frame->r;
	r.min = addpt(r.min, p);
	r.max = addpt(r.min, Pt(1, 1));

	// FIXME: This assumes color to be 24 bit RGB
	uchar bits[4];
	bits[2] = color >> 24;
	bits[1] = color >> 16;
	bits[0] = color >> 8;
	loadmemimage(frame, r, bits, sizeof bits);
}

void writeImagePixel(Image *dst, Point p, int color) {
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

void drawVerticalLine(Memimage *frame, int x, int ytop, int ybottom,
		      int color) {
	if (ytop < 0) {
		ytop = 0;
	}
	if (ytop > ybottom) return;

	for (int y = ytop; y < ybottom; y++) {
		writeMemimagePixel(frame, Pt(x, y), color);
	}
}

int addFog(int color, int depth) {
	int r = (color >> 24) & 255;
	int g = (color >> 16) & 255;
	int b = (color >> 8) & 255;
	double p = 0.0;
	if (depth > 100) p = (depth - 100) / 1000.0;
	r = (int)(r + (150 - r) * p);
	g = (int)(g + (170 - g) * p);
	b = (int)(b + (170 - b) * p);
	return (r << 24) + (g << 16) + (b << 8);
}

void render(Memimage *frame, double px, double py, double deg, int height, int horizon, int distance, int screenWidth, int screenHeight) {
	int *yBuffer;

	yBuffer = malloc(screenWidth * sizeof(int));
	for (int i = 0; i < screenWidth; i++)
		yBuffer[i] = screenHeight;

	double s = sin(deg);
	double c = cos(deg);

	double dz = 1.0;

	for (double z = 1; z < distance; z += dz) {
		double plx = -c * z - s * z;
		double ply = s * z - c * z;

		double prx = c * z - s * z;
		double pry = -s * z - c * z;

		double dx = (prx - plx) / screenWidth;
		double dy = (pry - ply) / screenWidth;
		plx += px;
		ply += py;
		double invz = 1. / z * 240;

		for (int i = 0; i < screenWidth; i++) {
			int offsetX = (int)plx & 1023;
			int offsetY = (int)ply & 1023;

			int heightOfHeightMap = hmap[offsetX][offsetY] & 255;
			int heightOnScreen = (int) ((height - heightOfHeightMap) * invz + horizon);
			int color = addFog(cmap[offsetX][offsetY], z);
			drawVerticalLine(frame, i, heightOnScreen, yBuffer[i], color);

			if (heightOnScreen < yBuffer[i])
				yBuffer[i] = heightOnScreen;

			plx += dx;
			ply += dy;
		}

		dz += 0.005;
	}

	free(yBuffer);
}

void drawBackground(Memimage *frame) {
	memfillcolor(frame, bgColor);
}

void clearScreen(Memimage *frame) {
	memfillcolor(frame, DTransparent);
}

void updateCamera(void) {
	camerax += 2 * cos(cameraAngle);
	cameray += 2 * sin(cameraAngle);
	cameraAngle += 0.001;
}

void redraw(void) {
	Memimage *frame;
	int nbits;
	uchar *bits;

	/* Back buffer for the double buffering algorithm  */
	frame = allocmemimage(screen->r, screen->chan);

	/* raw memory for the load/unload */
	nbits = bytesperline(frame->r, frame->depth) * Dy(frame->r);
	bits = malloc(nbits);

	/* Game Logic */
	updateCamera();

	/* Rendering routines */
	clearScreen(frame);
	drawBackground(frame);
	render(frame, camerax, cameray, cameraAngle, cameraHeight, cameraHorizon, cameraDistance, Dx(screen->r), Dy(screen->r));

	/* move back buffer to front buffer */
	unloadmemimage(frame, frame->r, bits, nbits);
	loadimage(screen, frame->r, bits, nbits);
	
	free(bits);
	freememimage(frame);
}

void eresized(int new) {
	if (new && getwindow(display, Refnone) < 0)
		sysfatal("can't reattach to window");

	redraw();
}

int loadImage(char *file, int buf[WSIZE][HSIZE]) {
	Memimage *mi;
	Point p;
	int fd;

	if ((fd = open(file, OREAD)) < 0) return -1;

	if ((mi = readmemimage(fd)) == nil) return -1;

	for (int i = 0; i < WSIZE; i++) {
		for (int j = 0; j < HSIZE; j++) {
			p.x = i;
			p.y = j;
			buf[i][j] = getPixelColor(mi, p);
		}
	}

	close(fd);
	freememimage(mi);
	return 0;
}

void drawString(char *msg) {
	string(screen, addpt(screen->r.min, Pt(4, 4)),
	       display->black, ZP,
	       font, msg);
}

void main(int argc, char *argv[]) {
	Event ev;

	if (argc != 3)
		sysfatal("Please provide colormap and heightmap file");

	memimageinit();

	/* Initiate graphics and mouse */
	if (initdraw(nil, nil, "Voxel Space") < 0)
		sysfatal("initdraw failed: %r");

	/* Load cmap and hmap images */
	if (loadImage(argv[1], cmap) < 0) sysfatal("LoadImage cmap: %r");
	if (loadImage(argv[2], hmap) < 0) sysfatal("LoadImage hmap: %r");

	/* Trigger a initial resize */
	eresized(0);

	einit(Emouse | Ekeyboard);

	/* Timer for the event loop */
	int Etimer = etimer(0, 1000 / 30);
	char cameradbgmsg[200];

	/* Main event loop */
	for (;;) {
		switch (eread(Emouse|Ekeyboard|Etimer, &ev)) {
		case Emouse:
			if ((ev.mouse.buttons & 4) && 
				(emenuhit(3, &ev.mouse, &menu) == 0)) {
				exits(nil);
			}
			break;
		case Ekeyboard:
			switch (ev.kbdc) {
				case 'w':
					camerax -= 2 * sin(cameraAngle);
					cameray -= 2 * cos(cameraAngle);
					break;
				case 'a':
					cameraAngle += 0.01;
					break;
				case 's':
					camerax -= -2 * sin(cameraAngle);
					cameray -= -2 * cos(cameraAngle);
					break;
				case 'd':
					cameraAngle -= 0.01;
					break;
				case 'q':
					cameraHeight -= 1;
					break;
				case 'e':
					cameraHeight += 1;
					break;
			}
			break;
		default: // Etimer
			redraw();
			break;
		}

		char msg[100];
		sprintf(msg, "Use WASD to move around");
		drawString(msg);
	}
}
