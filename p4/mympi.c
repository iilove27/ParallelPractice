#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpi.h>
#include <string.h>


#define         X_RESN  200      /* x resolution */
#define         Y_RESN  200       /* y resolution */
#define         limit   1e-7


typedef double vector[X_RESN];

int size, myid;
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
char            *window_name = (char *)"Heat Transfer", *display_name = NULL;
GC              gc;
unsigned
long            valuemask = 0;
XGCValues       values;
Display         *display;
XSizeHints      size_hints;

XSetWindowAttributes attr[1];

MPI_Datatype MPI_VECTOR;

vector *globalmat;

void initialize(vector globalmat[], vector localmat[], vector localstorage[], int batch)
{
    if (myid == 0)
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
    for (int i = 0; i < batch; i++)
        for (int j = 0; j < batch; j++)
        {
            localmat[j][i] = 0.0;
            localstorage[j][i] = 0.0;
        }
    
    if (size > 1) {
        MPI_Scatter(globalmat, batch, MPI_VECTOR, localmat, batch, MPI_VECTOR, 0, MPI_COMM_WORLD);
    }
    
}


int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    
    if (myid == 0)
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
    }
    
    int globalcontin = 0;
    int count = 0;
    int contin;
    int batch;
    double start_time;
    double end_time;
    vector *localmat;
    vector *localstorage;
    vector *recBuf1;
    vector *recBuf2;
    
    batch = Y_RESN / size;
    
    MPI_Type_contiguous(X_RESN, MPI_DOUBLE, &MPI_VECTOR);
    MPI_Type_commit(&MPI_VECTOR);
    localmat = (vector *)malloc(batch * sizeof(vector));
    localstorage = (vector *)malloc(batch * sizeof(vector));
    recBuf1 = (vector *)malloc(sizeof(vector));
    recBuf2 = (vector *)malloc(sizeof(vector));
    
    
    
    start_time = MPI_Wtime();
    
    if (myid == 0)
    {
        globalmat = (vector *)malloc(Y_RESN * sizeof(vector));
    }
    
    initialize(globalmat, localmat, localstorage, batch);
    
    if (size == 1) /* Sequential */
    {
        do {
            contin = 0;
            for (int i = 1; i < X_RESN-1; i++)
                for (int j = 1; j < Y_RESN-1; j++)
                {
                    localstorage[j][i] = (globalmat[j-1][i]
                                          + globalmat[j][i+1]
                                          + globalmat[j+1][i]
                                          + globalmat[j][i-1]) / 4;
                }
            
            for (int i = 1; i < X_RESN-1; i++)
                for (int j = 1; j < Y_RESN-1; j++)
                {
                    if ((globalmat[j][i] - localstorage[j][i] > limit) | (globalmat[j][i] - localstorage[j][i] < -limit))   contin += 1;
                    globalmat[j][i] = localstorage[j][i]; /* This ensures WALLs' temperature won't change */
                }
            if (count % 1000 == 0) {
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
        do
        {
            globalcontin = 0;
            contin = 0;
            if (myid != size-1)
            {
                MPI_Send(localmat[batch-1], 1, MPI_VECTOR, myid+1, 0, MPI_COMM_WORLD);
            }
            
            if (myid != 0)
            {
                MPI_Recv(recBuf1[0], 1, MPI_VECTOR, myid-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(localmat[0], 1, MPI_VECTOR, myid-1, 0, MPI_COMM_WORLD);
            }
            
            if (myid != size-1)
            {
                MPI_Recv(recBuf2[0], 1, MPI_VECTOR, myid+1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            
            if (myid == 0)
            {
                for(int j = 1; j < batch-1; j++)
                    for(int i = 1; i < X_RESN-1; i++)
                    {
                        localstorage[j][i] = (localmat[j-1][i]
                                              + localmat[j][i+1]
                                              + localmat[j+1][i]
                                              + localmat[j][i-1]) / 4;
                    }
                
                for (int i = 1; i < X_RESN-1; i++)
                {
                    localstorage[batch-1][i] = (localmat[batch-2][i]
                                                + localmat[batch-1][i-1]
                                                + localmat[batch-1][i+1]
                                                + recBuf2[0][i]) / 4;
                }
                
                for (int j = 1; j < batch; j++)
                    for (int i = 1; i < X_RESN-1; i++)
                    {
                        if ((localmat[j][i] - localstorage[j][i] > limit) | (localmat[j][i] - localstorage[j][i] < -limit))   contin += 1;
                        localmat[j][i] = localstorage[j][i];
                    }
            }
            
            else if (myid == size-1)
            {
                for (int i = 1; i < X_RESN-1; i++)
                {
                    localstorage[0][i] = (localmat[1][i]
                                          + localmat[0][i-1]
                                          + localmat[0][i+1]
                                          + recBuf1[0][i]) / 4;
                }
                
                for (int j = 1; j < batch-1; j++)
                    for (int i = 1; i < X_RESN-1; i++)
                    {
                        localstorage[j][i] = (localmat[j-1][i]
                                              + localmat[j][i+1]
                                              + localmat[j+1][i]
                                              + localmat[j][i-1]) / 4;
                    }
                
                for (int j = 0; j < batch-1; j++)
                    for (int i = 1; i < X_RESN-1; i++)
                    {
                        if ((localmat[j][i] - localstorage[j][i] > limit) | (localmat[j][i] - localstorage[j][i] < -limit))   contin += 1;
                        localmat[j][i] = localstorage[j][i];
                    }
                
            }
            
            else
            {
                for (int i = 1; i < X_RESN-1; i++)
                {
                    localstorage[0][i] = (localmat[1][i]
                                          + localmat[0][i-1]
                                          + localmat[0][i+1]
                                          + recBuf1[0][i]) / 4;
                    localstorage[batch-1][i] = (localmat[batch-2][i]
                                                + localmat[batch-1][i-1]
                                                + localmat[batch-1][i+1]
                                                + recBuf2[0][i]) / 4;
                }
                
                for (int j = 1; j < batch-1; j++)
                    for (int i = 1; i < X_RESN-1; i++)
                    {
                        localstorage[j][i] = (localmat[j-1][i]
                                              + localmat[j][i+1]
                                              + localmat[j+1][i]
                                              + localmat[j][i-1]) / 4;
                    }
                
                for (int j = 0; j < batch; j++)
                    for (int i = 1; i < X_RESN-1; i++)
                    {
                        if ((localmat[j][i] - localstorage[j][i] > limit) | (localmat[j][i] - localstorage[j][i] < -limit))   contin += 1;
                        localmat[j][i] = localstorage[j][i];
                    }
            }
            
            MPI_Allreduce(&contin, &globalcontin, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
            MPI_Gather(localmat, batch, MPI_VECTOR, globalmat, batch, MPI_VECTOR, 0, MPI_COMM_WORLD);
            if (myid == 0)
            {
                if (count % 1000 == 0)
                {
                    for (int i = 0; i < X_RESN; i++)
                        for (int j = 0; j < Y_RESN; j++)
                        {
                            XSetForeground(display, gc, color[(int)(globalmat[i][j]/5.0f)].pixel);
                            XDrawPoint(display, win, gc, j, i);
                        }
                }
                count += 1;
                
            }
            
        } while (globalcontin > 0);
        
    }
    
    
    end_time = MPI_Wtime();
    if (myid == 0)
    {
        printf("TIME SPENT IS %f second(s).", end_time - start_time);
    }
    MPI_Finalize();
    return 0;
}



