/* B-Spline Generator                                        */
/* Compile with  gcc main.c -o bspline -lGL -lGLU -lglut -lm */

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define SCREEN_HEIGHT 480
#define SCREEN_WIDTH 640
#define MAX_POINTS 50

/* Points */
struct Point {
    int x, y;
};

/* Globals */
struct Point points[MAX_POINTS];

int left_button_down = 0;
int current_point = -1;
int num_points = 0;
float knot[MAX_POINTS];

/* Declarations */
float b_spline(int k, int m, float t);
void remove_point(int x, int y);
void mouse_input(int button, int state, int x, int y);
void keyboard_input(unsigned char key, int x, int y);
void init();
void draw_screen();
void draw_point(int x, int y);
int update_point(int x, int y);
void rebuild_array(int begin);
int build_knots(int m, int L);
void mouse_move_input(int x, int y);

/* Main generates window, initializes keys*/
int main(int argc, char **argv)
{	
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
    glutInitWindowPosition(100, 100);

    glutCreateWindow("B-Spline");
    
    glutMouseFunc(mouse_input);
    glutDisplayFunc(draw_screen);
    glutKeyboardFunc(keyboard_input);
    glutMotionFunc(mouse_move_input); 
    
    init();
    
    glutMainLoop();
    
    return 0;
}

/* Initialize graphic settings */
void init()
{	
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glColor3f(0.0, 0.0, 0.0);
    glPointSize(6.0);
    glLineWidth(2.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 640.0, 0.0, 480.0);
}

/* Points */
void draw_screen()
{
    int i, m = 4;
    double t, sum_x, sum_y, x_old, y_old;
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    glColor3f(0.0, 0.0, 1.0);
    
    /* draw points and connecting lines */
    for (i = 0; i < num_points; i++) {
        draw_point(points[i].x, points[i].y);
    
        if (i == 0) {
            x_old = points[i].x;
            y_old = points[i].y;
        }
        
        glBegin(GL_LINES);
            glVertex2i(x_old, y_old);
            glVertex2i(points[i].x, points[i].y);
        glEnd();
        
        x_old = points[i].x;
        y_old = points[i].y;
    }
    
    /* exit if no knot vector */
    if (!build_knots(m, num_points - 1)) {
        glutSwapBuffers();
        glFlush();
        
        return;
    }
    
    /* reset last point variables */
    x_old = points[0].x;
    y_old = points[0].y;
    
    glColor3f(0.0, 1.0, 0.0);
    
    /* draw curve */
    for (t = knot[m-1] ; t <= knot[num_points]; t += .05) {
        sum_x = 0;
        sum_y = 0;
        
        for (i = 0; i < num_points; i++) {
            sum_x += points[i].x * b_spline(i, m, t);
            sum_y += points[i].y * b_spline(i, m, t);
        }	
        
        
        if (sum_x == 0.0 || sum_y == 0.0)
            continue;
        
        glBegin(GL_LINES);
            glVertex2i(x_old, y_old);
            glVertex2i(sum_x, sum_y);
        glEnd();
        
        x_old = sum_x;
        y_old = sum_y;
    }
        
    glutSwapBuffers();
    glFlush();
}

/* remove the selected point */
void remove_point(int x, int y)
{
    int i;
    
    for (i = 0; i < num_points; i++) {
        if (points[i].x >= (x - 3) && points[i].x <= (x + 3) && 
            points[i].y >= (y - 3) && points[i].y <= (y + 3)) {
            rebuild_array(i); 
        }
    }
}

/* rebuild point array after removing point */
void rebuild_array(int begin)
{
    int j;
    
    num_points--;
    for (j = begin; j < num_points; j++) {
        points[j].x = points[j + 1].x;
        points[j].y = points[j + 1].y;
    }
}

/* move the selected point */
int update_point(int x, int y)
{
    int i;
    int point_found = 0;

    for (i = 0; i < num_points; i++) {
        if (points[i].x >= (x - 3) && points[i].x <= (x + 3) && 
            points[i].y >= (y - 3) && points[i].y <= (y + 3)) {	
            
            points[i].x = x;
            points[i].y = y;
            current_point = i;
            point_found = 1;
        } 
    }
    return point_found;
}

/* intercept mouse input */
void mouse_input(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int point_found = update_point(x, SCREEN_HEIGHT - y);
        if (point_found)        
            left_button_down = 1;

        if (!point_found && num_points < MAX_POINTS) {
            points[num_points].x = x;
            points[num_points].y = SCREEN_HEIGHT - y;
            current_point = num_points;
            num_points++;
        }
    } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        remove_point(x, SCREEN_HEIGHT - y);
    } else if (button==GLUT_LEFT_BUTTON && state == GLUT_UP) {
        left_button_down = 0;
    }

    draw_screen();
}

/* intercept mouse input to move a point */
void mouse_move_input(int x, int y)
{
    if (left_button_down) {
        points[current_point].x = x;
        points[current_point].y = SCREEN_HEIGHT - y;
        draw_screen();
    }
}

/* calculates b-spline location */
float b_spline(int k, int m, float t)
{
    float denom1, denom2, sum = 0.0;
    
    if (m == 1)
        return (t >= knot[k] && t < knot[k+1]);
    
    denom1 = knot[k+m -1] - knot[k];

    if (denom1 != 0.0)
        sum = (t - knot[k]) * b_spline(k, m - 1, t) / denom1;

    denom2 = knot[k + m] - knot[k + 1];
    
    if (denom2 != 0.0)
        sum += (knot[k + m] - t) * b_spline(k + 1, m - 1, t) / denom2;

    return sum;
}

/* Builds knots used to calculate splines */
int build_knots(int m, int L)
{
    int i;
    
    if (L < (m - 1)) return 0;
    
    for (i = 1; i <= L + m; i++) {
        if (i < m) knot[i] = 0.0;
        else if (i <= L) knot[i] = i - m + 1;
        else knot[i] = L - m + 2; 
    }
    return 1;
}

/* Draws a point */
void draw_point(int x, int y)
{
    glBegin(GL_POINTS);
        glVertex2i(x, y);
    glEnd();
}

/* Intercepts keyboard input to quit */
void keyboard_input(unsigned char key, int x, int y)
{
    int i;
    switch (key) {
        case 'q': 
        case 'Q': 
            exit(0); 
            break;
       }
}
