// Author: Arnold Meijster (a.meijster@rug.nl)

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#ifndef NOVIEW

#include <GL/glut.h>
// glut requires the use of global variables. Since the process is forked and they are declared as statis, this is
// should not be an issue
static const char *windowTitle;
static int imageWidth, imageHeight;
static int windowWidth, windowHeight, redrawNeeded = 1;
static int threshold, thresholdMode = 0;
static int originX, originY, showOriginMode = 0;

static uint8_t *image;
static GLubyte *displayBuffer;
static unsigned char LUT[256][3];
static int winPosX, winPosY;

static void fillBuffer(void) {
  if (redrawNeeded == 0) {
    return;
  }
  // fill buffer using nearest neighbour interpolation
  double dx = (double)imageWidth / windowWidth;
  double dy = (double)imageHeight / windowHeight;
  int dbidx = 0;
  for (int i = 0; i < windowHeight; i++) {
    int y = i * dy;  // truncates to int!
    y = imageHeight - 1 - y;
    int yIdx = y * imageWidth;
    for (int j = 0; j < windowWidth; j++) {
      int x = j * dx;
      int pixval = image[x + yIdx];
      if (showOriginMode && x == originX && y == originY) {
        displayBuffer[dbidx++] = 255;
        displayBuffer[dbidx++] = 0;
        displayBuffer[dbidx++] = 0;
      } else {
        displayBuffer[dbidx++] = LUT[pixval][0];
        displayBuffer[dbidx++] = LUT[pixval][1];
        displayBuffer[dbidx++] = LUT[pixval][2];
      }
    }
  }
  redrawNeeded = 0;
}

static void greyLUT() {  // linear greyscale Look Up Table (LUT)
  for (int i = 0; i < 256; i++) {
    LUT[i][0] = LUT[i][1] = LUT[i][2] = i;
  }
  showOriginMode = 0;
  thresholdMode = 0;
}

static void invertImage() {
  int npixels = imageWidth * imageHeight;
  for (int i = 0; i < npixels; i++) {
    image[i] = 255 - image[i];
  }
  threshold = 255 - threshold;
}

static void randomLUT() {  // random colour Look Up Table (LUT)
  for (int i = 0; i < 256; i++) {
    LUT[i][0] = random() & 255;
    LUT[i][1] = random() & 255;
    LUT[i][2] = random() & 255;
  }
  thresholdMode = 0;
}

static void getMinMax(int *minimum, int *maximum) {
  int min, max, npixels = imageWidth * imageHeight;
  min = max = image[0];
  for (int i = 0; i < npixels; i++) {
    min = (image[i] < min ? image[i] : min);
    max = (image[i] > max ? image[i] : max);
  }
  *minimum = min;
  *maximum = max;
}

static void contrastStretchLUT() {  // contrast stretch
  int min, max;
  getMinMax(&min, &max);
  double scale = 255.0 / (max - min);
  printf("Linear contrast stretch: min=%d, max=%d, stretchfactor=%lf\n", min, max, scale);
  for (int i = 0; i < 256; i++) {
    LUT[i][0] = LUT[i][1] = LUT[i][2] = (0.5 + scale * (i - min));
  }
  thresholdMode = 0;
}

static void histEqLUT() {
  printf("Histogram Equalization\n");
  int *histogram = calloc(sizeof(int), 256);
  int npixels = imageWidth * imageHeight;
  for (int i = 0; i < npixels; i++) {
    histogram[image[i]]++;
  }
  double sum = 0.0;
  for (int i = 0; i < 256; i++) {
    sum += histogram[i];
    double cdf = sum / npixels;
    LUT[i][0] = LUT[i][1] = LUT[i][2] = 0.5 + 255 * cdf;
  }
  free(histogram);
  thresholdMode = 0;
}

static void thresholdLUT() {
  printf("threshold = %d\n", threshold);
  for (int i = 0; i < 256; i++) {
    LUT[i][0] = LUT[i][1] = LUT[i][2] = 255 * (i >= threshold);
  }
  thresholdMode = 1;
}

static void init(void) {
  int min, max;
  getMinMax(&min, &max);
  threshold = (min + max) / 2;  // initial threshold
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
    displayBuffer = realloc(displayBuffer, 3 * w * h);
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
  double dx = (double)windowWidth / imageWidth;
  double dy = (double)windowHeight / imageHeight;
  double scale = (dx > dy ? dx : dy);
  reshapeDisplayBuffer(scale * imageWidth, scale * imageHeight);
  glutReshapeWindow(windowWidth, windowHeight);
}

static void showOrigin() {
  showOriginMode = 1;
  redrawNeeded = 1;
}

static void keyboard(unsigned char key, int x, int y) {
  redrawNeeded = 1;
  key = toupper(key);
  int min, max;
  switch (key) {
    case 27:
    case 'Q':
      free(image);
      free(displayBuffer);
      exit(EXIT_SUCCESS);
    case 'O':
      showOrigin();
      break;
    case 'A':  // Aspect ratio
      aspectRatio();
      break;
    case 'C':
      contrastStretchLUT();
      break;
    case 'H':
      histEqLUT();
      break;
    case 'I':
      invertImage();
      break;
    case 'S':  // image stats
      getMinMax(&min, &max);
      printf("width=%d, height=%d, minimal grey value=%d, maximal grey value=%d\n", imageWidth, imageHeight, min, max);
      redrawNeeded = 0;
      break;
    case 'T':
      thresholdLUT();
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
        displayBuffer = realloc(displayBuffer, 3 * windowWidth * windowHeight);
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
        dx = (double)imageWidth / windowWidth;
        dy = (double)imageHeight / windowHeight;
        y = y * dy;  // truncates to int
        x = x * dx;  // truncates to int
        int idx = y * imageWidth + x;
        printf("im[%d][%d] = %d\n", y, x, LUT[image[idx]][0]);
        return;                 // instead of break: saves an unnecessary redisplay
      case GLUT_RIGHT_BUTTON:   // right button clicks are ignored
      case GLUT_MIDDLE_BUTTON:  // middle button clicks are ignored
        return;                 // instead of break: saves an unnecessary redisplay
      case 3:
        // mouse wheel scroll up
        if (glutGetModifiers() == GLUT_ACTIVE_CTRL) {
          // increase threshold in threshold mode
          if (thresholdMode) {
            threshold++;
            thresholdLUT();
            redrawNeeded = 1;
          }
        } else {
          // zoom in
          reshape(1.1 * windowWidth, 1.1 * windowHeight);
          glutReshapeWindow(windowWidth, windowHeight);
        }
        break;
      case 4:  // mouse wheel scroll up (zoom out)
        if (glutGetModifiers() == GLUT_ACTIVE_CTRL) {
          // decrease threshold in threshold mode
          if (thresholdMode) {
            threshold--;
            thresholdLUT();
            redrawNeeded = 1;
          }
        } else {
          // zoom out
          reshape(0.9 * windowWidth, 0.9 * windowHeight);
          glutReshapeWindow(windowWidth, windowHeight);
        }
        break;
    }
  }
  glutPostRedisplay();
}

static void displayProcess() {
  displayBuffer = malloc(3 * imageWidth * imageHeight);
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

void glutGreyScaleViewer(uint8_t *values, int width, int height, int orX, int orY, const char *title) {
  // assume cheap 1366x768 screen
  if ((width > 1366) || (height > 768)) {
    winPosX = winPosY = 0;
  } else {
    if (winPosX + width + 16 > 1366) {
      // 16 seems reasonable for window frame width
      winPosX = 0;
      winPosY += height / 2;
      if (winPosY > 768) {
        winPosX = winPosY = 0;
      }
    }
  }
  if (fork() == 0) {
    originX = orX;
    originY = orY;
    windowTitle = title;
    image = values;
    imageWidth = width;
    imageHeight = height;
    windowWidth = width;
    windowHeight = height;
    displayProcess();
    exit(EXIT_SUCCESS);
  }
  // only parent process gets here
  free(values);
  winPosX += width + 16;
}

#else
void glutGreyScaleViewer(uint8_t *values, int width, int height, int orX, int orY, const char *title) {
  fprintf(stderr,
          "warning: Greyscale image viewer for '%s' could not be opened, since the program was compiled with the "
          "NOVIEW flag.\n",
          title);
}
#endif
