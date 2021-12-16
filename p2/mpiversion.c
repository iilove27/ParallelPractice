#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <stdlib.h>


#define         X_RESN  800      /* x resolution */
#define         Y_RESN  800       /* y resolution */
typedef struct complextype
{
    float real, imag;
} Compl;

int main (int argc, char *argv[])
{
    int myid, numproc;
    int depth = atoi(argv[1]);
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &numproc);
    double t1, t2;
    t1 = MPI_Wtime();
    /* Handle remainder situation */
    int job_width;
    if (Y_RESN % numproc) job_width = Y_RESN / numproc + 1;
    else job_width = Y_RESN / numproc;
    
    /* Calculate and draw points */


    int bigMat [Y_RESN][X_RESN];
    int i, j, k;
    Compl   z, c;
    float   lengthsq, temp;
    int matrix [job_width][X_RESN];

    for(i=0; i < X_RESN; i++)
        for(j=myid*job_width; j < (1+myid)*job_width; j++) {
            z.real = z.imag = 0.0;
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
            matrix[j-myid*job_width][i] = k;
        }

    if (numproc > 1) {
        if (myid == 0) {
            for (int j = 0; j < X_RESN; j++)
                for (int k = 0; k < job_width; k++){
                    bigMat[k][j] = matrix[k][j];
                }
            if (numproc > 2) {
                for (int i = 1; i < numproc-1; i++) {
                    int recbuf [job_width][X_RESN];
                    MPI_Recv(recbuf, job_width*X_RESN, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    for (int j = 0; j < X_RESN; j++)
                        for (int k = 0; k < job_width; k++){
                            bigMat[k+job_width*i][j] = recbuf[k][j];
                        }
                }
                int recbuf [job_width][X_RESN];
                MPI_Recv(recbuf, job_width*X_RESN, MPI_INT, numproc-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int j = 0; j < X_RESN; j++){
                    for (int k = 0; k < Y_RESN-(numproc-1)*job_width; k++) {
                        bigMat[(numproc-1)*job_width + k][j] = recbuf[k][j];
                    }
                }
            }
            else {
                int recbuf [job_width][X_RESN];
                MPI_Recv(recbuf, job_width*X_RESN, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int j = 0; j < X_RESN; j++){
                    for (int k = 0; k < Y_RESN-job_width; k++){
                        bigMat[job_width+k][j] = recbuf[k][j];
                    }
                }
            }
            
        }
        else {
            MPI_Send(matrix, job_width*X_RESN, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }
    else {
        for (int i = 0; i < X_RESN; i++)
            for (int j = 0; j < Y_RESN; j++){
                bigMat[j][i] = matrix[j][i];
            }
    }
    t2 = MPI_Wtime();
    if (myid == 0) 
	{
		printf("Done with %i processes.\n", numproc);
		printf("Time spent was %f seconds.\n", t2 - t1);
	
	}
    MPI_Finalize();

    if (myid == 0) {
        Window          win;                            /* initialization for a window */
        unsigned
        int             width, height,                  /* window size */
        x, y,                           /* window position */
        border_width,                   /*border width in pixels */
        display_width, display_height,  /* size of screen */
        screen;                         /* which screen */
        char            *window_name = "Mandelbrot Set", *display_name = NULL;
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

        /* Mandlebrot variables */


        /* connect to Xserver */

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

        for (int j = 0; j < X_RESN; j++){
            for (int k = 0; k < Y_RESN; k++) {
                    XSetForeground (display, gc,  1024*1024*(bigMat[k][j]%256));
                    XDrawPoint(display, win, gc, k, j);
            }
        }
        XFlush (display);
        sleep (30);

    }

    /* Program Finished */
    return 0;
}

