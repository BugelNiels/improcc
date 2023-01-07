// Author: Arnold Meijster (a.meijster@rug.nl)
#include <GL/glut.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

// glut requires the use of global variables. Since the process is forked and they are declared as statis, this is
// should not be an issue
static const char *windowTitle;
static int imageWidth, imageHeight;
static int windowWidth, windowHeight, redrawNeeded = 1;

static uint8_t *red;
static uint8_t *green;
static uint8_t *blue;
static GLubyte *displayBuffer;
static unsigned char LUT[256][3];
static int winPosX, winPosY;

static void fillBuffer(void) {
  if (redrawNeeded == 0) {
    return;
  }
  // fill buffer using nearest neighbour interpolation
  double dx = (double)imageWidth/windowWidth;
  double dy = (double)imageHeight/windowHeight;
  int dbidx = 0;
  for (int i = 0; i < windowHeight; i++) {
    int y = i*dy;  // truncates to int!
    y = imageHeight - 1 - y;
    int yIdx = y*imageWidth;
    for (int j = 0; j < windowWidth; j++) {
      int x = j*dx;
      int r = red[x + yIdx];
      int g = green[x + yIdx];
      int b = blue[x + yIdx];
      displayBuffer[dbidx++] = LUT[r][0];
      displayBuffer[dbidx++] = LUT[g][1];
      displayBuffer[dbidx++] = LUT[b][2];
    }
  }
  redrawNeeded = 0;
}

static void greyLUT() {  // linear greyscale Look Up Table (LUT)
  for (int i = 0; i < 256; i++) {
    LUT[i][0] = LUT[i][1] = LUT[i][2] = i;
  }
}

static void invertImage() {
  int npixels = imageWidth*imageHeight;
  for (int i = 0; i < npixels; i++) {
    red[i] = 255 - red[i];
    green[i] = 255 - green[i];
    blue[i] = 255 - blue[i];
  }
}

static void randomLUT() {  // random colour Look Up Table (LUT)
  for (int i = 0; i < 256; i++) {
    LUT[i][0] = random() & 255;
    LUT[i][1] = random() & 255;
    LUT[i][2] = random() & 255;
  }
}

static void init(void) {
  greyLUT();
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glShadeModel(GL_FLAT);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

static void display(void) {
  fillBuffer();
  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2i(0, 0);
  glDrawPixels(windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, displayBuffer);
  glFlush();
}

static void reshapeDisplayBuffer(int w, int h) {
  if ((windowWidth != w) || (windowHeight != h)) {
    windowWidth = w;
    windowHeight = h;
    displayBuffer = realloc(displayBuffer, 3*w*h);
    redrawNeeded = 1;
  }
}

static void reshape(int w, int h) {
  reshapeDisplayBuffer(w, h);  // reshapes only if needed
  glViewport(0, 0, (GLsizei)w, (GLsizei)h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, w, 0, h, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
}

static void aspectRatio() {
  double dx = (double)windowWidth/imageWidth;
  double dy = (double)windowHeight/imageHeight;
  double scale = (dx > dy ? dx : dy);
  reshapeDisplayBuffer(scale*imageWidth, scale*imageHeight);
  glutReshapeWindow(windowWidth, windowHeight);
}

static void keyboard(unsigned char key, int x, int y) {
  redrawNeeded = 1;
  key = toupper(key);
  switch (key) {
    case 27:
    case 'Q':
      free(red);
      free(green);
      free(blue);
      free(displayBuffer);
      exit(EXIT_SUCCESS);
    case 'A':  // Aspect ratio
      aspectRatio();
      break;
    case 'I':
      invertImage();
      break;
    case 'S':
      printf("width=%d, height=%d\n", imageWidth, imageHeight);
      redrawNeeded = 0;
      break;
    case 'F':  // False coloring
      randomLUT();
      printf("Random LUT\n");
      break;
    case 'G':
      greyLUT();
      break;
    case 'R':
      greyLUT();
      if ((windowWidth != imageWidth) || (windowHeight != imageHeight)) {
        windowWidth = imageWidth;
        windowHeight = imageHeight;
        displayBuffer = realloc(displayBuffer, 3*windowWidth*windowHeight);
        glutReshapeWindow(windowWidth, windowHeight);
      }
      break;
  }
  glutPostRedisplay();
}

static void mouse(int button, int state, int x, int y) {
  double dx;
  double dy;
  if (state == GLUT_DOWN) {
    switch (button) {
      case GLUT_LEFT_BUTTON:
        dx = (double)imageWidth/windowWidth;
        dy = (double)imageHeight/windowHeight;
        y = y*dy;  // truncates to int
        x = x*dx;  // truncates to int
        int idx = y*imageWidth + x;
        printf("im[%d][%d] = (%d,%d,%d)\n", y, x, LUT[red[idx]][0], LUT[green[idx]][1], LUT[blue[idx]][2]);
        return;                 // instead of break: saves an unnecessary redisplay
      case GLUT_RIGHT_BUTTON:   // right button clicks are ignored
      case GLUT_MIDDLE_BUTTON:  // middle button clicks are ignored
        return;                 // instead of break: saves an unnecessary redisplay
      case 3:
        reshape(1.1*windowWidth, 1.1*windowHeight);
        glutReshapeWindow(windowWidth, windowHeight);
        break;
      case 4:
        // zoom out
        reshape(0.9*windowWidth, 0.9*windowHeight);
        glutReshapeWindow(windowWidth, windowHeight);
        break;
    }
  }
  glutPostRedisplay();
}

static void displayProcess() {
  displayBuffer = malloc(3*imageWidth*imageHeight);
  char *argv[1];
  int argc = 1;
  argv[0] = "improc";
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(imageWidth, imageHeight);
  glutInitWindowPosition(winPosX, winPosY);
  glutCreateWindow(windowTitle);
  init();
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutDisplayFunc(display);
  glutMainLoop();
}

void glutRgbViewer(uint8_t *rValues, uint8_t *gValues, uint8_t *bValues, int width, int height, const char *title) {
  // assume cheap 1366x768 screen
  if ((width > 1366) || (height > 768)) {
    winPosX = winPosY = 0;
  } else {
    if (winPosX + width + 16 > 1366) {
      // 16 seems reasonable for window frame width
      winPosX = 0;
      winPosY += height/2;
      if (winPosY > 768) {
        winPosX = winPosY = 0;
      }
    }
  }
  if (fork() == 0) {
    windowTitle = title;
    imageWidth = width;
    imageHeight = height;
    windowWidth = width;
    windowHeight = height;
    red = rValues;
    green = gValues;
    blue = bValues;
    displayProcess();
    exit(EXIT_SUCCESS);
  }
  // only parent process gets here
  free(rValues);
  free(gValues);
  free(bValues);
  winPosX += width + 16;
}
