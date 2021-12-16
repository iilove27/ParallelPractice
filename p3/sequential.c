#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define         X_RESN  800      /* x resolution */
#define         Y_RESN  800       /* y resolution */



typedef double vector[2];

const double G = 1000;
double t_change = 0.002;
double *masses;
clock_t start, end;
int total_num, num_threads, batch, total_steps;


XColor color;
  Colormap colormap;
  char black[] = "#000000";

Window          win;                            /* initialization for a window */
unsigned
int             width, height,                  /* window size */
x, y,                           /* window position */
border_width,                   /*border width in pixels */
display_width, display_height,  /* size of screen */
screen;                         /* which screen */
char            *window_name = (char *)"Nbody", *display_name = NULL;
GC              gc;
unsigned
long            valuemask = 0;
XGCValues       values;
Display         *display;
XSizeHints      size_hints;
XSetWindowAttributes attr[1];




vector *velocity;
vector *position;
vector *all_position;
vector *force;


void generate()
{
    /* first let all mass to be 5 */
    double mass = 5;
    for (int i = 0; i < total_num; i++)
    {
        masses[i] = mass;
        all_position[i][0] = rand() % 800;
        all_position[i][1] = rand() % 800;   /* no collusion needed */
        velocity[i][0] = 0.0;
        velocity[i][1] = 0.0;
        force[i][0] = 0.0;
        force[i][1] = 0.0;
    } 
}


void cal_force()
{
	int global_index = 0;
	double counter_mass = 0;
	double r, r_3, F;
	double vertical, horizontal;

	for (int i = 0; i < total_num; i++)
	{
		global_index = i;
		double my_mass = masses[global_index];
		/* Initially, set force to zero
		otherwise force will be accumulated on force calculated in previous loop.*/
		for (int counter = 0; counter < total_num; counter++)
		{
			if (counter != global_index)
			{
				counter_mass = masses[counter];
				horizontal = all_position[global_index][0] - all_position[counter][0];
				vertical = all_position[global_index][1] - all_position[counter][1];
				r = sqrt(pow(horizontal, 2) + pow(vertical, 2)); 
                /*r_2 = horizontal * horizontal + vertical * vertical; */

				r_3 = pow(r, 3)+5e-2;
				F = G * my_mass * counter_mass / r_3; /* Actually this is F/r */
				force[global_index][0] -= F * horizontal;
				force[global_index][1] -= F * vertical;
			}
		}
	}

}

void gogogo()
{
    int global_index;
    for (int i = 0; i < total_num; i++)
    {
        global_index = i;
        velocity[global_index][0] += force[global_index][0] * t_change / masses[global_index];
        velocity[global_index][1] += force[global_index][1] * t_change / masses[global_index];
        all_position[global_index][0] += t_change * velocity[global_index][0];
        if (all_position[global_index][0] > 800)
        {
            all_position[global_index][0] = 800;
            velocity[global_index][0] *= -1;
            velocity[global_index][1] *= -1;


        }

        else if (all_position[global_index][0] < 0)
        {
            all_position[global_index][0] = 0;
            velocity[global_index][0] *= -1;
            velocity[global_index][1] *= -1;


        }
        all_position[global_index][1] += t_change * velocity[global_index][1];
        if (all_position[global_index][1] > 800)
        {
            all_position[global_index][1] = 800;
            velocity[global_index][0] *= -1;
            velocity[global_index][1] *= -1;


        }

        else if (all_position[global_index][1] < 0)
        {
            all_position[global_index][1] = 0;
            velocity[global_index][0] *= -1;
            velocity[global_index][1] *= -1;


        }
        
		force[global_index][0] = 0.0;
		force[global_index][1] = 0.0;
    }
    for (int i = 0; i < total_num; i++)
    {
        global_index = i;
        printf("Position %f, %f. globalindex %i \n", all_position[global_index][0], all_position[global_index][1], global_index);
    }

}





int main(int argc, char* argv[])
{
	if (  (display = XOpenDisplay (display_name)) == NULL ) {
            fprintf (stderr, "drawon: cannot connect to X server %s\n",
                     XDisplayName (display_name) );
        }
        
    /* get screen size */
    
    screen = DefaultScreen (display);
    display_width = DisplayWidth (display, screen);
    display_height = DisplayHeight (display, screen);
    
    /* set window size */
    
    width = X_RESN;
    height = Y_RESN;
    
    /* set window position */
    
    x = 0;
    y = 0;
    
    /* create opaque window */
    colormap = DefaultColormap(display, 0);
    XParseColor(display, colormap, black, &color);
    XAllocColor(display, colormap, &color);
    border_width = 4;
    win = XCreateSimpleWindow (display, RootWindow (display, screen),
                               x, y, width, height, border_width,
                               WhitePixel(display, screen), color.pixel);
    
    size_hints.flags = USPosition|USSize;
    size_hints.x = x;
    size_hints.y = y;
    size_hints.width = width;
    size_hints.height = height;
    size_hints.min_width = 300;
    size_hints.min_height = 300;
    
    XSetNormalHints (display, win, &size_hints);
    XStoreName(display, win, window_name);
    
    /* create graphics context */
    
    gc = XCreateGC (display, win, valuemask, &values);
    
    XSetBackground (display, gc, WhitePixel(display, screen));
    XSetForeground (display, gc, BlackPixel (display, screen));
    XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);
    attr[0].backing_store = Always;
    attr[0].backing_planes = 1;
    attr[0].backing_pixel = BlackPixel(display, screen);
    
    XChangeWindowAttributes(display, win, CWBackingStore | CWBackingPlanes | CWBackingPixel, attr);
    
    XMapWindow (display, win);
    XSync(display, 0);


	total_num = atoi(argv[1]);
    total_steps = atoi(argv[2]);


	masses = (double *)malloc(total_num * sizeof(double)); /* masses and all_position and velocity are GLOBAL variables. */
    all_position = (vector *)malloc(total_num * sizeof(vector));
    velocity = (vector *)malloc(total_num * sizeof(vector));
    force = (vector *)malloc(total_num * sizeof(vector));




	
    start = clock();
    generate();	

    for (int step = 0; step < total_steps; step++)
    {
    	cal_force();
        gogogo();
        XClearArea(display, win, 0, 0, 800, 800, 0);
    	for (int i = 0; i < total_num; i++)
    	{
            XSetForeground(display, gc, WhitePixel(display, screen));

    		XDrawArc(display, win, gc, all_position[i][0]-1, all_position[i][1]-1, 2, 2, 0, 360*64);
    	}
    	XFlush(display);

    }
    end = clock();
    double time = (double) (end-start) / CLOCKS_PER_SEC;
    printf("Elapse time is %f second(s)", time);
	return 0;
}



















