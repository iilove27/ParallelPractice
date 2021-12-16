#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <iostream>
#include <time.h>


#define         X_RESN  800      /* x resolution */
#define         Y_RESN  800       /* y resolution */
typedef struct complextype
{
    float real, imag;
} Compl;

int calc_depth(int j, int i, int depth);
pthread_mutex_t mutexsum;
struct args {
    int start;
    int depth;
    int end;
    int job_width;
    int id;
    Window win;
    GC gc;
    Display *display;

};
Window          win;                            /* initialization for a window */
unsigned
int             width, height,                  /* window size */
x, y,                           /* window position */
border_width,                   /*border width in pixels */
display_width, display_height,  /* size of screen */
screen;                         /* which screen */
char            *window_name = (char *)"Mandelbrot Set", *display_name = NULL;
GC              gc;
unsigned
long            valuemask = 0;
XGCValues       values;
Display         *display;
XSizeHints      size_hints;
Pixmap          bitmap;
XPoint          points[800];
FILE            *fp, *fopen ();
char            str[100];

XSetWindowAttributes attr[1];
void *fillMat(void *input)
{
    pthread_mutex_lock(&mutexsum);

    int localmat [((struct args*)input)->job_width][X_RESN];
    
    for (int i = 0; i < X_RESN; i++)
        for (int j = (((struct args*)input)->id)*(((struct args*)input)->job_width); j < (((struct args*)input)->id+1)*(((struct args*)input)->job_width); j++)
        {
            localmat[j-(((struct args*)input)->id)*(((struct args*)input)->job_width)][i] = calc_depth(j, i, ((struct args*)input)->depth);
            
        }

    for (int i = 0; i < X_RESN; i++){
        for (int j = 0; j < ((struct args*)input)->job_width; j++) {
            XSetForeground (((struct args*)input)->display, ((struct args*)input)->gc,  1024*1024*(localmat[j][i]%256));
            XDrawPoint(((struct args*)input)->display, ((struct args*)input)->win, ((struct args*)input)->gc, j+(((struct args*)input)->id)*(((struct args*)input)->job_width), i);
        }
    }
    pthread_mutex_unlock(&mutexsum);
    return NULL;
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
    
    border_width = 4;
    win = XCreateSimpleWindow (display, RootWindow (display, screen),
                               x, y, width, height, border_width,
                               BlackPixel (display, screen), WhitePixel (display, screen));
    
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
    
    XSetBackground (display, gc, 0X0000FF00);
    XSetForeground (display, gc, BlackPixel (display, screen));
    XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);
    
    attr[0].backing_store = Always;
    attr[0].backing_planes = 1;
    attr[0].backing_pixel = BlackPixel(display, screen);
    
    XChangeWindowAttributes(display, win, CWBackingStore | CWBackingPlanes | CWBackingPixel, attr);
    
    XMapWindow (display, win);
    XSync(display, 0);
    
    
    
    
    pthread_mutex_init(&mutexsum, NULL);
    int depth = atoi(argv[2]);
    clock_t t = clock();
    int num = atoi(argv[1]);
    /* Handle situation where number of threads does not divide width */
    int job_width = Y_RESN / num;
    if (Y_RESN % job_width)
    {
        job_width += 1;
    }
    
    pthread_t threads[num];
    for (int id = 0; id < num; id++)
    {
        int y_start, y_end;
        y_start = id * job_width;
        
        if (id == num - 1) y_end = Y_RESN;
        else y_end = (id + 1) * job_width;
        
        
        struct args *argument = (struct args *)malloc(sizeof(struct args));
        argument ->start = y_start;
        argument ->end = y_end;
        argument ->depth = depth;
        argument ->job_width = job_width;
        argument ->id = id;
        argument ->win = win;
        argument ->display = display;
        argument ->gc = gc;
        pthread_create(&threads[id], NULL, fillMat, (void *)argument);
        
    }
    
    for (int id = 0; id < num; id++)
    {
        pthread_join(threads[id], NULL);
    }
    
    
    
 
    
    
    
    /* Mandlebrot variables */
    
    
    /* connect to Xserver */
    

    
    /*for (int j = 0; j < X_RESN; j++){
        for (int k = 0; k < Y_RESN; k++) {
            XSetForeground (display, gc,  1024*1024*(bigMat[k][j]%256));
            XDrawPoint(display, win, gc, k, j);
        }
    }*/
   
    XFlush (display);
    t = clock()-t;
    printf("Done with %i thread(s)", num);
    printf("Time spent is %f second(s)", (float) t / CLOCKS_PER_SEC);
    pthread_exit(NULL);
    
}





int calc_depth(int j, int i, int depth)
{
    Compl z, c;
    int k;
    float   lengthsq, temp;
    z.real = 0.0;
    z.imag = 0.0;
    c.real = ((float) j - 400.0)/200.0;               /* scale factors for 800 x 800 window */
    c.imag = ((float) i - 400.0)/200.0;
    k = 0;
    do  {                                             /* iterate for pixel color */
        temp = z.real*z.real - z.imag*z.imag + c.real;
        z.imag = 2.0*z.real*z.imag + c.imag;
        z.real = temp;
        lengthsq = z.real*z.real+z.imag*z.imag;
        k++;
    } while (lengthsq < 4.0 && k < depth);
    return k;
}

