#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define         X_RESN  200      /* x resolution */
#define         Y_RESN  200       /* y resolution */
#define limit 1e-5


typedef double vector[X_RESN];
int contin = 100;
int myid;
int numthreads;
int thread_count;

int *countarray;

int count = 0;

int batch;
time_t t1, t2;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

XColor color[256];
Colormap colormap;
char white[] = "#FFFFFF";
Window          win;                            /* initialization for a window */
unsigned
int             width, height,                  /* window size */
x, y,                           /* window position */
border_width,                   /*border width in pixels */
display_width, display_height,  /* size of screen */
screen;                         /* which screen */
char            *window_name = (char *)"Heat Distribution", *display_name = NULL;
GC              gc;
unsigned
long            valuemask = 0;
XGCValues       values;
Display         *display;
XSizeHints      size_hints;

XSetWindowAttributes attr[1];

vector *globalmat;
vector *storage;
vector temp[X_RESN];




void initialize()
{
    for (int i = 0; i < X_RESN; i++)
        for (int j = 0; j < Y_RESN; j++)
            globalmat[i][j] = 0.0;

    for (int i = 0; i < X_RESN; i++)  
    {
        globalmat[0][i] = 20.0;
        globalmat[Y_RESN-1][i] = 20.0;
    }

    for (int i = 0; i < Y_RESN; i++)
    {
        globalmat[i][0] = globalmat[i][X_RESN-1] = 20.0;
    }

    /* Fire place */

    for (int i = X_RESN / 4; i < 3 * X_RESN / 4; i++)
    	globalmat[0][i] = 100.0;

   
}


void *job(void *arg)
{
	int myid;
	myid = *((int *) arg);

	if (myid == 0)
	{
		for(int j = 1; j < batch; j++)
            for(int i = 1; i < X_RESN-1; i++)
            {
                storage[j][i] = (globalmat[j-1][i]
                                    + globalmat[j][i+1]
                                    + globalmat[j+1][i]
                                    + globalmat[j][i-1]) / 4;
            }
	}

	else if (myid == numthreads-1)
	{
		for (int j = Y_RESN - batch; j < Y_RESN-1; j++)
			for (int i = 1; i < X_RESN-1; i++)
			{
                storage[j][i] = (globalmat[j-1][i]
                    + globalmat[j][i+1]
                    + globalmat[j+1][i]
                    + globalmat[j][i-1]) / 4;
			}
	}

	else
	{
		for (int j = batch * myid; j < batch * (myid+1); j++)
			for (int i = 1; i < X_RESN-1; i++)
			{
				storage[j][i] = (globalmat[j-1][i]
                                    + globalmat[j][i+1]
                                    + globalmat[j+1][i]
                                    + globalmat[j][i-1]) / 4;
			}

	}
	return NULL;
}

void *fill(void *arg)
{
	int myid;
	myid = *((int *) arg);

	if (myid == 0)
	{
		for(int j = 1; j < batch; j++)
            for(int i = 1; i < X_RESN-1; i++)
            {
            	if ((globalmat[j][i] - storage[j][i] > limit) | (globalmat[j][i] - storage[j][i] < -limit)) contin += 1;
                globalmat[j][i] = storage[j][i];
            }
	}

	else if (myid == numthreads-1)
	{
		for (int j = Y_RESN - batch; j < Y_RESN-1; j++)
			for (int i = 1; i < X_RESN-1; i++)
			{
            	if ((globalmat[j][i] - storage[j][i] > limit) | (globalmat[j][i] - storage[j][i] < -limit)) contin += 1;
	            globalmat[j][i] = storage[j][i];
			}
	}

	else
	{
		for (int j = batch * myid; j < batch * (myid+1); j++)
			for (int i = 1; i < X_RESN-1; i++)
			{
            	if ((globalmat[j][i] - storage[j][i] > limit) | (globalmat[j][i] - storage[j][i] < -limit)) contin += 1;
                globalmat[j][i] = storage[j][i];
			}

	}
	return NULL;
}




int main(int argc, char *argv[])
{
	if (  (display = XOpenDisplay (display_name)) == NULL ) {
            fprintf (stderr, "drawon: cannot connect to X server %s\n",
                     XDisplayName (display_name) );
        }
        
        /* get screen size */
        
    screen = DefaultScreen (display);
    display_width = DisplayWidth (display, screen);
    display_height = DisplayHeight (display, screen);
    

    colormap = DefaultColormap(display, 0);

    int i;
	for (i=0; i<20; ++i)
	{
	    color[i].green = rand()%65535;
	    color[i].red = rand()%65535;
	    color[i].blue = rand()%65535;
	    color[i].flags = DoRed | DoGreen | DoBlue;
	    XAllocColor(display, colormap, &color[i]);
	}



    /* set window size */
    
    width = X_RESN;
    height = Y_RESN;
    
    /* set window position */
    
    x = 0;
    y = 0;
    
    /* create opaque window */
    border_width = 4;
    win = XCreateSimpleWindow (display, RootWindow (display, screen),
                               x, y, width, height, border_width, BlackPixel(display, screen),
                               WhitePixel(display, screen));
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
    
    XSetBackground (display, gc, BlackPixel(display, screen));
    XSetForeground (display, gc, WhitePixel (display, screen));
    XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);
    attr[0].backing_store = Always;
    attr[0].backing_planes = 1;
    attr[0].backing_pixel = BlackPixel(display, screen);
    
    XChangeWindowAttributes(display, win, CWBackingStore | CWBackingPlanes | CWBackingPixel, attr);
    
    XMapWindow (display, win);
    XSync(display, 0);





    numthreads = atoi(argv[1]);
    batch = Y_RESN / numthreads;

    globalmat = (vector *)malloc(Y_RESN * sizeof(vector));
    storage = (vector *)malloc(Y_RESN * sizeof(vector));

    countarray = (int *)malloc(numthreads * sizeof(int));
    for (int i = 0; i < numthreads; i++)
    	countarray[i] = 0;

    t1 = time(NULL);
    initialize();


    if (numthreads == 1)
    {
    	do {
    		contin = 0;
         	/* this is one loop */
            for (int i = 1; i < X_RESN-1; i++)
                for (int j = 1; j < Y_RESN-1; j++)
                {
                    storage[j][i] = (globalmat[j-1][i]
                                        + globalmat[j][i+1]
                                        + globalmat[j+1][i]
                                        + globalmat[j][i-1]) / 4;
                }

            for (int i = 1; i < X_RESN-1; i++)
                for (int j = 1; j < Y_RESN-1; j++)
                {
                    if ((globalmat[j][i] - storage[j][i] > limit) | (globalmat[j][i] - storage[j][i] < -limit))   contin += 1;
                    globalmat[j][i] = storage[j][i]; /* This ensures WALLs' temperature won't change */
                }
            if (count % 100 == 0) {
            XClearArea(display, win, 0, 0, 200, 200, 0);
            for (int i = 0; i < X_RESN; i++)
                for (int j = 0; j < Y_RESN; j++)
                {
                    XSetForeground(display, gc, color[(int)(globalmat[i][j]/5.0f)].pixel);
                    XDrawPoint(display, win, gc, j, i);
                }
            }
            count += 1;
           } while (contin > 0);
    }

    else
    {

    	int indexs[numthreads];
    	for (int i = 0; i < numthreads; i++)	indexs[i] = i;

    	pthread_t compute[numthreads];
    	pthread_t fillin[numthreads];

    	do 
    	{
    		contin = 0;
    		for (int id = 0; id < numthreads; id++)
    		{
    			pthread_create(&compute[id], NULL, job, (void *)&indexs[id]);
    		}

    		for (int id = 0; id < numthreads; id++)
    			pthread_join(compute[id], NULL);

    		for (int id = 0; id < numthreads; id++)
    			pthread_create(&fillin[id], NULL, fill, (void *)&indexs[id]);

    		for (int id = 0; id < numthreads; id++)
    			pthread_join(fillin[id], NULL);

    		 if (count % 100 == 0) {
            XClearArea(display, win, 0, 0, 200, 200, 0);
            for (int i = 0; i < X_RESN; i++)
                for (int j = 0; j < Y_RESN; j++)
                {
                    XSetForeground(display, gc, color[(int)(globalmat[i][j]/5.0f)].pixel);
                    XDrawPoint(display, win, gc, j, i);
                }
            } 
            count += 1;
            printf("%i \n", count);
    	} while (contin > 0);
    }



    t2 = time(NULL);
    printf("Elapse time: %f seconds", difftime(t2,t1));


	return 0;
}



