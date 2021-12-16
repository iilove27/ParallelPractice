#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpi.h>
#include <string.h>


#define         X_RESN  800      /* x resolution */
#define         Y_RESN  800       /* y resolution */



typedef double vector[2];

const double G = 1000;
int size, myid;
double t_change = 0.002;

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

vector *velocity = NULL;
MPI_Datatype MPI_VECTOR;

void generate(vector position[], int total_num, double masses[], vector velocity[])
{
            srand(time(NULL));

    /* first let all mass to be 3 */
    double mass = 5;
    for (int i = 0; i < total_num; i++)
    {
        masses[i] = mass;
        position[i][0] = rand() % 800;
        position[i][1] = rand() % 800;   /* no collusion needed */
        
        velocity[i][0] = 0.0;
        velocity[i][1] = 0.0;
    }
    
}


void initialize(double masses[], vector position[], vector velocity[], vector local_velocity[], int total_num, int batch)
{
    if (myid == 0)
    {
            generate(position, total_num, masses, velocity);
    }
    
    MPI_Bcast(masses, total_num, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(position, total_num, MPI_VECTOR, 0, MPI_COMM_WORLD);
}

void force(vector position[], int local_index, double masses[], vector local_force[], int batch, int total_num)
{
    double counter_mass;
    int global_index;
    double r, r_3, F;
    double vertical, horizontal;
    global_index = local_index + myid * batch;
    double my_mass = masses[global_index];

    /* Initially, set force to zero
     otherwise force will be accumulated on force calculated in previous loop.*/
    local_force[local_index][0] = 0.0;
    local_force[local_index][1] = 0.0;
    for (int counter = 0; counter < total_num; counter++)
    {
        if (counter != global_index)
        {
            counter_mass = masses[counter];
            horizontal = position[global_index][0] - position[counter][0];
            vertical = position[global_index][1] - position[counter][1];
            r = sqrt(pow(horizontal, 2) + pow(vertical, 2));
            r_3 = pow(r, 3) + 5e-2;
            F = G * my_mass * counter_mass / r_3; /* Actually this is F/r */
            local_force[local_index][0] -= F * horizontal;
            local_force[local_index][1] -= F * vertical;
        }
    }
}

void gogogo(vector local_force[], vector local_position[], vector local_velocity[], int batch, double masses[])
{
    int global_index;
    for (int i = 0; i < batch; i++)
    {
        global_index = i + myid * batch;
        local_velocity[i][0] += local_force[i][0] * t_change / masses[global_index];
        local_velocity[i][1] += local_force[i][1] * t_change / masses[global_index];
        local_position[i][0] += t_change * local_velocity[i][0];
        if (local_position[i][0] > 800)
        {
            local_position[i][0] = 800;
            local_velocity[i][0] = 0;
            local_velocity[i][1] = 0;


        }

        else if (local_position[i][0] < 0)
        {
            local_position[i][0] = 0;
            local_velocity[i][0] = 0;
            local_velocity[i][1] = 0;


        }
        local_position[i][1] += t_change * local_velocity[i][1];
        if (local_position[i][1] > 800)
        {
            local_position[i][1] = 800;
            local_velocity[i][1] = 0;
            local_velocity[i][0] = 0;

        }

        else if (local_position[i][1] < 0)
        {
            local_position[i][1] = 0;
            local_velocity[i][1] = 0;
            local_velocity[i][0] = 0;


        }
        
        local_force[i][0] = 0.0;
        local_force[i][1] = 0.0;
    }
    for (int i = 0; i < batch; i++)
    {
        global_index = i + myid * batch;
        printf("Position %f, %f. rank %i, globalindex %i velocity_x %f velocity_y %f \n", local_position[i][0], local_position[i][1], myid, global_index, local_velocity[i][0], local_velocity[i][1]);
    }
    
    
}

int main(int argc, char *argv[])
{
    
    
    
    int total_num = atoi(argv[1]);
    int batch;
    int total_steps = atoi(argv[2]);
    double *masses;
    double start_time;
    double end_time;
    vector *all_position;
    vector *local_position;
    vector *local_force;
    vector *local_velocity;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    batch = total_num / size;
    
    MPI_Type_contiguous(2, MPI_DOUBLE, &MPI_VECTOR);
    MPI_Type_commit(&MPI_VECTOR);
    
    
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
    
    
    masses = (double *)malloc(total_num * sizeof(double));
    all_position = (vector *)malloc(total_num * sizeof(vector));
    local_position = all_position + myid * batch;
    local_force = (vector *)malloc(batch * sizeof(vector));
    local_velocity = (vector *)malloc(batch * sizeof(vector));
    
    if (myid == 0)
        velocity = (vector *)malloc(total_num * sizeof(vector));
    start_time = MPI_Wtime();
    
    initialize(masses, all_position, velocity, local_velocity, total_num, batch);
    
    if (myid == 0)
    {
        for (int i = 0; i < total_num; i++)
        {
            XSetForeground(display, gc, WhitePixel(display, screen));
            XDrawArc(display, win, gc, all_position[i][0]-1, all_position[i][1]-1, 2, 2, 0, 360*64);
        }
        XFlush(display);
    }
    for (int step = 0; step < total_steps; step++)
    {
        for (int local_index = 0; local_index < batch; local_index++)
        {
            force(all_position, local_index, masses, local_force, batch, total_num);
        }
        gogogo(local_force, local_position, local_velocity, batch, masses);
        
        MPI_Allgather(MPI_IN_PLACE, batch, MPI_VECTOR, all_position, batch, MPI_VECTOR, MPI_COMM_WORLD);
        if (myid == 0)
        {
            XClearArea(display, win, 0, 0, 800, 800, 0);
            for (int i = 0; i < total_num; i++)
            {
                XSetForeground(display, gc, WhitePixel(display, screen));
                XDrawArc(display, win, gc, all_position[i][0]-1, all_position[i][1]-1, 2, 2, 0, 360*64);
                
            }
            XFlush(display);
        }
    }

    end_time = MPI_Wtime();
    if (myid == 0)
    {
        printf("TIME SPENT IS %f second(s).", end_time - start_time);
    }
    MPI_Finalize();
    return 0;
}











