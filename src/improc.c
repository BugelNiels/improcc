#include "improc.h"

#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*binaryOp)(int, int);

// declaration of image viewer
void glutGreyScaleViewer(uint8_t *values, int width, int height, int originX, int originY, const char *title);
void glutRgbViewer(uint8_t *rValues, uint8_t *gValues, uint8_t *bValues, int width, int height, const char *title);

/** Util ****************************************************/

static void warning(const char *format, ...) {
  fprintf(stderr, "Warning: ");
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

static void fatalError(const char *format, ...) {
  fprintf(stderr, "Fatal error: ");
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  exit(EXIT_FAILURE);
}

static void *safeMalloc(int sz) {
  void *p = malloc(sz);
  if (p == NULL) {
    fatalError("safeMalloc(%d) failed.\n", sz);
  }
  return p;
}

static void *safeCalloc(int sz) {
  void *p = calloc(sz, 1);
  if (p == NULL) {
    fatalError("safeCalloc(%d) failed.\n", sz);
  }
  return p;
}

static int **allocIntMatrix(int width, int height) {
  int **matrix = safeMalloc(height*sizeof(int *) + width*height*sizeof(int));
  int *p = (int *)(matrix + height);
  for (int y = 0; y < height; y++) {
    matrix[y] = p + width*y;
  }
  return matrix;
}

static char *getFileNameExtension(const char *path) {
  char *extension = strrchr(path, '.');
  return (extension == NULL ? NULL : extension + 1);
}

/** Image Domain ****************************************************/

static ImageDomain initImageDomain(int minX, int maxX, int minY, int maxY) {
  int width = 1 + maxX - minX;   // note: domain is [minX..maxX], bounds included
  int height = 1 + maxY - minY;  // note: domain is [min\Y..maxY], bounds included

  if ((width <= 0) || (height <= 0)) {
    fatalError("Attempting to initialise image with width=%d, height=%d. Image dimensions must be greater than 0.\n",
               width, height);
  }
  ImageDomain domain;
  domain.minX = minX;
  domain.maxX = maxX;
  domain.minY = minY;
  domain.maxY = maxY;
  return domain;
}

ImageDomain getIntImageDomain(IntImage image) { return image.domain; }
static ImageDomain *getIntImageDomainPtr(IntImage *image) { return &(image->domain); }
static ImageDomain *getRgbImageDomainPtr(RgbImage *image) { return &(image->domain); }

int getMinX(ImageDomain domain) { return domain.minX; }

int getMaxX(ImageDomain domain) { return domain.maxX; }

int getMinY(ImageDomain domain) { return domain.minY; }

int getMaxY(ImageDomain domain) { return domain.maxY; }

void getImageDomainValues(ImageDomain domain, int *minX, int *maxX, int *minY, int *maxY) {
  *minX = domain.minX;
  *maxX = domain.maxX;
  *minY = domain.minY;
  *maxY = domain.maxY;
}

int getWidth(ImageDomain domain) {
  return 1 + domain.maxX - domain.minX;  // note: domain is [minX..maxX], bounds included
}

int getHeight(ImageDomain domain) {
  return 1 + domain.maxY - domain.minY;  // note: domain is [minY..maxY], bounds included
}

void checkDomain(int x, int y, int minX, int maxX, int minY, int maxY) {
  if ((x < minX) || (x > maxX) || (y < minY) || (y > maxY)) {
    fatalError("Attempt to access pixel (x,y)=(%d,%d) which is outside the image domain [%d..%d]x[%d..%d].\n", x, y,
               minX, maxX, minY, maxY);
  }
}

int isInDomain(ImageDomain domain, int x, int y) {
  int minX, maxX, minY, maxY;
  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);
  return !((x < minX) || (x > maxX) || (y < minY) || (y > maxY));
}

static void flipDomainHorizontal(ImageDomain *domain) {
  int minX = domain->minX;
  domain->minX = -domain->maxX;
  domain->maxX = -minX;
}

static void flipDomainVertical(ImageDomain *domain) {
  int minY = domain->minY;
  domain->minY = -domain->maxY;
  domain->maxY = -minY;
}

static ImageDomain padDomain(ImageDomain domain, int top, int right, int bottom, int left) {
  int minX, maxX, minY, maxY;
  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);
  ImageDomain paddedDomain = initImageDomain(minX - left, maxX + right, minY - bottom, maxY + top);
  return paddedDomain;
}

/** Image ********************************************/

IntImage allocateIntImage(int width, int height, int minValue, int maxValue) {
  return allocateIntImageGrid(0, width - 1, 0, height - 1, minValue, maxValue);
}

IntImage allocateDefaultIntImage(int width, int height) { return allocateIntImage(width, height, INT_MIN, INT_MAX); }

IntImage allocateFromIntImage(IntImage image) {
  ImageDomain domain = getIntImageDomain(image);
  int minX, maxX, minY, maxY;
  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);
  int minRange, maxRange;
  getDynamicRange(image, &minRange, &maxRange);
  IntImage image2 = allocateIntImageGrid(minX, maxX, minY, maxY, minRange, maxRange);
  return image2;
}

IntImage allocateIntImageGrid(int minX, int maxX, int minY, int maxY, int minValue, int maxValue) {
  IntImage image;
  ImageDomain domain = initImageDomain(minX, maxX, minY, maxY);
  image.domain = domain;
  image.pixels = allocIntMatrix(getWidth(domain), getHeight(domain));
  // Dynamic range grey values
  image.minRange = minValue;
  image.maxRange = maxValue;
  return image;
}

IntImage copyIntImage(IntImage image) {
  ImageDomain domain = getIntImageDomain(image);
  int minX, maxX, minY, maxY;
  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);
  IntImage copy = allocateFromIntImage(image);
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      setIntPixel(&copy, x, y, getIntPixel(image, x, y));
    }
  }
  return copy;
}

void freeIntImage(IntImage image) { free(image.pixels); }

void getDynamicRange(IntImage image, int *minRange, int *maxRange) {
  *minRange = image.minRange;
  *maxRange = image.maxRange;
}

void getMinMax(IntImage image, int *minimalValue, int *maximalValue) {
  int minX, maxX, minY, maxY, minVal, maxVal;
  getImageDomainValues(getIntImageDomain(image), &minX, &maxX, &minY, &maxY);
  minVal = maxVal = getIntPixel(image, minX, minY);
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      int val = getIntPixel(image, x, y);
      minVal = (val < minVal ? val : minVal);
      maxVal = (val > maxVal ? val : maxVal);
    }
  }
  *minimalValue = minVal;
  *maximalValue = maxVal;
}

inline int getIntPixel(IntImage image, int x, int y) {
#if FAST
  return image.pixels[y - image.domain.minY][x - image.domain.minX];
#else
  int minX, maxX, minY, maxY;
  getImageDomainValues(image.domain, &minX, &maxX, &minY, &maxY);
  checkDomain(x, y, minX, maxX, minY, maxY);
  return image.pixels[y - minY][x - minX];
#endif
}

inline void setIntPixel(IntImage *image, int x, int y, int greyValue) {
#if FAST
  image->pixels[y - image->domain.minY][x - image->domain.minX] = greyValue;
#else
  if (greyValue < image->minRange) {
    warning("setIntPixel: value %d is outside dynamic range [%d,%d]: clamped to %d\n", greyValue, image->minRange,
            image->maxRange, image->minRange);
    greyValue = image->minRange;
  }
  if (greyValue > image->maxRange) {
    warning("setIntPixel: value %d is outside dynamic range [%d,%d]: clamped to %d\n", greyValue, image->minRange,
            image->maxRange, image->maxRange);
    greyValue = image->maxRange - 1;
  }

  int minX, maxX, minY, maxY;
  getImageDomainValues(image->domain, &minX, &maxX, &minY, &maxY);
  checkDomain(x, y, minX, maxX, minY, maxY);

  x -= minX;
  y -= minY;
  image->pixels[y][x] = greyValue;
#endif
}

void setAllIntPixels(IntImage *image, int greyValue) {
  if (greyValue < image->minRange) {
    warning("setAllIntPixels: value %d is outside dynamic range [%d,%d]: clamped to %d\n", greyValue, image->minRange,
            image->maxRange, image->minRange);
    greyValue = image->minRange;
  }
  if (greyValue > image->maxRange) {
    warning("setAllIntPixels: value %d is outside dynamic range [%d,%d]: clamped to %d\n", greyValue, image->minRange,
            image->maxRange, image->maxRange);
    greyValue = image->maxRange - 1;
  }
  int minX, maxX, minY, maxY;
  getImageDomainValues(getIntImageDomain(*image), &minX, &maxX, &minY, &maxY);
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      setIntPixel(image, x, y, greyValue);
    }
  }
}

void setDynamicRange(IntImage *image, int newMinRange, int newMaxRange) {
  image->minRange = newMinRange;
  image->maxRange = newMaxRange;
}

void printIntBuffer(IntImage image) {
  int minX, maxX, minY, maxY;
  getImageDomainValues(getIntImageDomain(image), &minX, &maxX, &minY, &maxY);
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      int gval = getIntPixel(image, x, y);
      printf("%d ", gval);
    }
    printf("\n");
  }
}

void printIntLatexTableToFile(FILE *out, IntImage image) {
  int minX, maxX, minY, maxY;
  getImageDomainValues(getIntImageDomain(image), &minX, &maxX, &minY, &maxY);
  fprintf(out, "\\begin{tabular}{|c|");
  for (int x = minX; x <= maxX; x++) {
    fprintf(out, "|c");
  }
  fprintf(out, "|}\n\\hline\n(x,y)");
  for (int x = minX; x <= maxX; x++) {
    fprintf(out, "&%d", x);
  }
  fprintf(out, "\\\\\n\\hline\n");
  fprintf(out, "\\hline\n");
  for (int y = minY; y <= maxY; y++) {
    fprintf(out, "%d", y);
    for (int x = minX; x <= maxX; x++) {
      int gval = getIntPixel(image, x, y);
      if ((y == 0) && (x == 0)) {  // origin
        fprintf(out, "&{\\bf %d}", gval);
      } else {
        fprintf(out, "&%d", gval);
      }
    }
    fprintf(out, "\\\\\\hline\n");
  }
  fprintf(out, "\\end{tabular}\n");
}

void printIntImageLatexTable(IntImage image) { printIntLatexTableToFile(stdout, image); }

static uint8_t *intImageToByteBuffer(IntImage image) {
  ImageDomain domain = getIntImageDomain(image);
  int minX, maxX, minY, maxY, width = getWidth(domain), height = getHeight(domain);
  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);

  uint8_t *buffer = safeMalloc(width*height);
  int idx = 0;
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      int gval = getIntPixel(image, x, y);
      buffer[idx++] = ((gval < 0) || (gval > 255) ? 0 : gval);
    }
  }
  return buffer;
}

static void rgbImageToByteBuffers(RgbImage image, uint8_t **rBuf, uint8_t **gBuf, uint8_t **bBuf) {
  ImageDomain domain = getRgbImageDomain(image);
  int minX, maxX, minY, maxY, width = getWidth(domain), height = getHeight(domain);
  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);

  *rBuf = safeMalloc(width*height);
  *gBuf = safeMalloc(width*height);
  *bBuf = safeMalloc(width*height);
  int idx = 0;
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      int r, g, b;
      getRgbPixel(image, x, y, &r, &g, &b);
      (*rBuf)[idx] = ((r < 0) || (r > 255) ? 0 : r);
      (*gBuf)[idx] = ((g < 0) || (g > 255) ? 0 : g);
      (*bBuf)[idx++] = ((b < 0) || (b > 255) ? 0 : b);
    }
  }
}

void displayIntImage(IntImage image, const char *windowTitle) {
  uint8_t *buffer = intImageToByteBuffer(image);
  ImageDomain domain = getIntImageDomain(image);
  int minX, maxX, minY, maxY, width = getWidth(domain), height = getHeight(domain);
  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);
  int min, max;
  getMinMax(image, &min, &max);
  if ((min < 0) || (max > 255)) {
    warning("displayIntImage: grey values are clamped in the image viewer to [0,255].\n");
  }
  glutGreyScaleViewer(buffer, width, height, -minX, -minY, windowTitle);
}

static IntImage applyFunctionIntImage(IntImage imageA, IntImage imageB, binaryOp operator) {
  IntImage result = allocateFromIntImage(imageA);
  int minX, maxX, minY, maxY;
  getImageDomainValues(getIntImageDomain(imageA), &minX, &maxX, &minY, &maxY);
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      int val = operator(getIntPixel(imageA, x, y), getIntPixel(imageB, x, y));
      setIntPixel(&result, x, y, val);
    }
  }
  return result;
}

static int maxOp(int a, int b) { return a > b ? a : b; }

static int minOp(int a, int b) { return a > b ? b : a; }

static int addOp(int a, int b) { return a + b; }

static int subtractOp(int a, int b) { return a - b; }

static int multiplyOp(int a, int b) { return a*b; }

static void compareDomains(IntImage imageA, IntImage imageB) {
  int minX1, maxX1, minY1, maxY1;
  getImageDomainValues(getIntImageDomain(imageA), &minX1, &maxX1, &minY1, &maxY1);
  int minX2, maxX2, minY2, maxY2;
  getImageDomainValues(getIntImageDomain(imageA), &minX2, &maxX2, &minY2, &maxY2);
  if (minX1 != minX2 || maxX1 != maxX2 || minY1 != minY2 || maxY1 != maxY2) {
    fatalError("Images do not have the same domain.");
  }
}

IntImage maxIntImage(IntImage imageA, IntImage imageB) {
  compareDomains(imageA, imageB);
  return applyFunctionIntImage(imageA, imageB, &maxOp);
}

IntImage minIntImage(IntImage imageA, IntImage imageB) {
  compareDomains(imageA, imageB);
  return applyFunctionIntImage(imageA, imageB, &minOp);
}

IntImage addIntImage(IntImage imageA, IntImage imageB) {
  compareDomains(imageA, imageB);
  return applyFunctionIntImage(imageA, imageB, &addOp);
}

IntImage subtractIntImage(IntImage imageA, IntImage imageB) {
  compareDomains(imageA, imageB);
  return applyFunctionIntImage(imageA, imageB, &subtractOp);
}

IntImage multiplyIntImage(IntImage imageA, IntImage imageB) {
  compareDomains(imageA, imageB);
  return applyFunctionIntImage(imageA, imageB, &multiplyOp);
}

IntImage applyLutIntImage(IntImage image, int *LUT, int LUTSize) {
  if (image.minRange < 0) {
    fatalError("applyLutIntImage: LUTs can only be applied to image with positive ");
  }
  if (image.maxRange > LUTSize) {
    fatalError("applyLutIntImage: LUT must be the same size as the dynamic range of the image");
  }

  IntImage resultImg = allocateFromIntImage(image);
  int minX, maxX, minY, maxY;
  getImageDomainValues(getIntImageDomain(image), &minX, &maxX, &minY, &maxY);
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      int val = LUT[getIntPixel(image, x, y)];
      setIntPixel(&resultImg, x, y, val);
    }
  }
  return resultImg;
}

/** Loading ********************************************/

IntImage padIntImage(IntImage image, int top, int right, int bottom, int left, int padValue) {
  ImageDomain domain = getIntImageDomain(image);
  ImageDomain paddedDomain = padDomain(domain, top, right, bottom, left);
  int minX, maxX, minY, maxY;
  getImageDomainValues(paddedDomain, &minX, &maxX, &minY, &maxY);
  int minRange, maxRange;
  getDynamicRange(image, &minRange, &maxRange);
  IntImage paddedImg = allocateIntImageGrid(minX, maxX, minY, maxY, minRange, maxRange);

  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      if (isInDomain(domain, x, y)) {
        setIntPixel(&paddedImg, x, y, getIntPixel(image, x, y));
      } else {
        setIntPixel(&paddedImg, x, y, padValue);
      }
    }
  }
  return paddedImg;
}

void translateIntImage(IntImage *image, int x, int y) {
  ImageDomain *domain = getIntImageDomainPtr(image);
  domain->minX += x;
  domain->maxX += x;
  domain->minY += y;
  domain->maxY += y;
}

void flipIntImageHorizontal(IntImage *image) {
  ImageDomain domain = getIntImageDomain(*image);
  int width = getWidth(domain);
  int height = getHeight(domain);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x <= width/2; x++) {
      int a = image->pixels[y][x];
      image->pixels[y][x] = image->pixels[y][width - x - 1];
      image->pixels[y][width - x - 1] = a;
    }
  }
  flipDomainHorizontal(getIntImageDomainPtr(image));
}

void flipIntImageVertical(IntImage *image) {
  ImageDomain domain = getIntImageDomain(*image);
  int width = getWidth(domain);
  int height = getHeight(domain);
  for (int y = 0; y <= height/2; y++) {
    for (int x = 0; x < width; x++) {
      int a = image->pixels[y][x];
      image->pixels[y][x] = image->pixels[height - y - 1][x];
      image->pixels[height - y - 1][x] = a;
    }
  }
  flipDomainVertical(getIntImageDomainPtr(image));
}

/** Loading ********************************************/

static unsigned short *loadImagePGM(const char *path, int *width, int *height, int *maxVal) {
  int magicNumber;
  FILE *imgFile = fopen(path, "r");
  if (fscanf(imgFile, "P%d\n", &magicNumber) != 1) {
    fclose(imgFile);
    fatalError("loadPgmImage: corrupt PGM: no magic number found.\n");
  }

  if ((magicNumber != 2) && (magicNumber != 5)) {
    fclose(imgFile);
    fatalError("Illegal magic number P%d found. Only P2 and P5 are valid PGM files.\n", magicNumber);
  }
  // skip comment lines
  char c;
  while ((c = fgetc(imgFile)) == '#') {
    do {
      c = fgetc(imgFile);
    } while ((c != EOF) && (c != '\n'));
    if (c == EOF) {
      fatalError("loadImagePGM: corrupt PGM file.\n");
    }
  }
  ungetc(c, imgFile);

  // read width and height
  if (fscanf(imgFile, "%d %d", width, height) != 2) {
    fatalError("loadImagePGM: corrupt PGM: no file dimensions found.\n");
  }
  if (fscanf(imgFile, "%d\n", maxVal) != 1) {
    fatalError("loadImagePGM: corrupt PGM file: no maximal grey value.\n");
  }
  if ((*maxVal < 0) || (*maxVal > 65535)) {
    fatalError("loadImagePGM: corrupt PGM: maximum grey value found is %d (must be in range [0..65535]).\n");
  }

  int npixels = (*width)*(*height);
  unsigned short *buffer = malloc(npixels*sizeof(short));
  if (magicNumber == 2) {
    int gval;
    for (int i = 0; i < npixels; i++) {
      if (fscanf(imgFile, "%d", &gval) != 1) {
        fatalError("loadPgmImage: corrupt PGM: non numeric data found in PGM image (P2 type).\n");
      }
      if ((gval < 0) || (gval > *maxVal)) {
        fatalError("loadImagePGM: pixel with grey value %d found. Valid dynamic range is [0..%d].\n", gval, *maxVal);
      }
      buffer[i] = gval;
    }
  } else {
    // magicnumber == 5
    if (*maxVal > 255) {  // 2 bytes per pixel, i.e. short
      if (fread(buffer, 2, npixels, imgFile) != npixels) {
        fatalError("loadImagePGM: corrupt PGM, file is truncated.\n");
      }
    } else {  // 1 byte per pixel
      unsigned char *buf = malloc(npixels*sizeof(unsigned char));
      if (fread(buf, 1, npixels, imgFile) != npixels) {
        fatalError("loadImagePGM: corrupt PGM, file is truncated.\n");
      }
      for (int i = 0; i < npixels; i++) {
        buffer[i] = buf[i];
      }
      free(buf);
    }
  }
  fclose(imgFile);
  return buffer;
}

static unsigned short *loadImagePBM(const char *path, int *width, int *height) {
  int magicNumber;
  FILE *imgFile = fopen(path, "r");
  if (fscanf(imgFile, "P%d\n", &magicNumber) != 1) {
    fclose(imgFile);
    fatalError("loadImagePBM: corrupt PBM file: no magic number found.\n");
  }

  if ((magicNumber != 1) && (magicNumber != 4)) {
    fclose(imgFile);
    fatalError("loadImagePBM: Illegal magic number P%d found. Only P1 and P4 are valid PBM files.\n", magicNumber);
  }
  // skip comment lines
  char c;
  while ((c = fgetc(imgFile)) == '#') {
    do {
      c = fgetc(imgFile);
    } while ((c != EOF) && (c != '\n'));
    if (c == EOF) {
      fatalError("loadImagePBM: corrupt PBM file.\n");
    }
  }
  ungetc(c, imgFile);

  // read width and height
  if (fscanf(imgFile, "%d %d\n", width, height) != 2) {
    fatalError("loadImagePBM: corrupt PBM: no file dimensions found.\n");
  }
  int npixels = (*width)*(*height);
  unsigned short *buffer = malloc(npixels*sizeof(short));
  if (magicNumber == 1) {
    for (int i = 0; i < npixels; i++) {
      char bit;
      do {
        if (fscanf(imgFile, "%c", &bit) != 1) {
          fatalError("loadImagePBM: corrupt PBM, , file is truncated.\n");
        }
      } while ((bit == ' ') || (bit == '\t') || (bit == '\n'));
      if ((bit != '0') && (bit != '1')) {
        fatalError("loadImagePBM: illegal character found.\n");
      }
      buffer[i] = bit - '0';
    }
  } else {
    int w = *width, h = *height, idx = 0;
    for (int i = 0; i < h; i++) {
      int j = 0;
      while (j < w) {
        unsigned char mask, byte;
        byte = fgetc(imgFile);
        for (mask = 128; mask && (j < w); mask >>= 1) {
          buffer[idx++] = (byte & mask ? 0 : 1);  // Note: in PBM 0 is white, 1 is black! (weird)
          j++;
        }
      }
    }
  }
  fclose(imgFile);
  return buffer;
}

IntImage loadIntImage(const char *path) {
  char *extension = getFileNameExtension(path);
  if (extension == NULL) {
    fatalError("loadIntImage: filename '%s' has no extension.\n", path);
  }
  FILE *f = fopen(path, "r");
  if (f == NULL) {
    fatalError("loadIntImage: failed to open file '%s'.\n", path);
  }
  fclose(f);

  int width, height, maxVal;
  unsigned short *buf = NULL;

  if (strcmp("pgm", extension) == 0) {
    buf = loadImagePGM(path, &width, &height, &maxVal);
  }
  if (strcmp("pbm", extension) == 0) {
    buf = loadImagePBM(path, &width, &height);
    maxVal = 255;
  }
  // copy buffer into image structure
  IntImage image = allocateIntImage(width, height, 0, maxVal);
  int idx = 0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      setIntPixel(&image, x, y, buf[idx++]);
    }
  }
  free(buf);
  return image;
}

/** Saving ********************************************/

static void saveImagePBMasP1(const char *path, int width, int height, uint8_t *buffer) {
  FILE *pbmFile = fopen(path, "w");
  if (pbmFile == NULL) {
    fatalError("saveImagePBMasP1: failed to open file '%s'.\n", path);
  }
  fprintf(pbmFile, "P1\n%d %d\n", width, height);
  int idx = 0;
  for (int i = 0; i < height; i++) {
    fprintf(pbmFile, "%d", (buffer[idx++] ? 0 : 1));
    for (int j = 1; j < width; j++) {
      fprintf(pbmFile, " %d", (buffer[idx++] ? 0 : 1));  // Note: in PBM 0 is white, 1 is black! (weird)
    }
    fprintf(pbmFile, "\n");
  }
  fclose(pbmFile);
}

static void saveImagePBMasP4(const char *path, int width, int height, uint8_t *buffer) {
  FILE *pbmFile = fopen(path, "w");
  if (pbmFile == NULL) {
    fatalError("saveImagePBMasP4: failed to open file '%s'.\n", path);
  }
  fprintf(pbmFile, "P4\n%d %d\n", width, height);

  int idx = 0;
  for (int i = 0; i < height; i++) {
    int j = 0;
    while (j < width) {
      unsigned char mask, byte = 0;
      for (mask = 128; mask && (j < width); mask >>= 1) {
        if (buffer[idx++] == 0) {  // Note: in PBM 0 is white, 1 is black! (weird)
          byte |= mask;
        }
        j++;
      }
      fputc(byte, pbmFile);
    }
  }
  fclose(pbmFile);
}

static void saveImagePGMasP5(const char *path, int width, int height, unsigned short *buffer) {
  FILE *pgmFile = fopen(path, "w");
  if (pgmFile == NULL) {
    fatalError("saveImagePGMasP5: failed to open file '%s'.\n", path);
  }
  fprintf(pgmFile, "P5\n%d %d\n", width, height);

  int npixels = (width)*(height);
  unsigned short max = buffer[0];
  for (int i = 0; (i < npixels) && (max < 65535); i++) {
    max = (buffer[i] > max ? buffer[i] : max);
  }
  fprintf(pgmFile, "%d\n", max);
  if (max > 255) {
    fwrite(buffer, 2, npixels, pgmFile);
  } else {
    unsigned char *buf = malloc(npixels);
    for (int i = 0; i < npixels; i++) {
      buf[i] = buffer[i];
    }
    fwrite(buf, 1, npixels, pgmFile);
    free(buf);
  }
  fclose(pgmFile);
}

static void saveImagePGMasP2(const char *path, int width, int height, unsigned short *buffer) {
  FILE *pgmFile = fopen(path, "w");
  if (pgmFile == NULL) {
    fatalError("saveImagePGMasP2: failed to open file '%s'.\n", path);
  }
  fprintf(pgmFile, "P2\n%d %d\n", width, height);

  int npixels = (width)*(height);
  unsigned short max = buffer[0];
  for (int i = 0; (i < npixels) && (max < 65535); i++) {
    max = (buffer[i] > max ? buffer[i] : max);
  }
  fprintf(pgmFile, "%d\n", max);
  int idx = 0;
  for (int i = 0; i < height; i++) {
    fprintf(pgmFile, "%d", buffer[idx++]);
    for (int j = 1; j < width; j++) {
      fprintf(pgmFile, " %d", buffer[idx++]);
    }
    fprintf(pgmFile, "\n");
  }
  fclose(pgmFile);
}

static void saveIntImagePGM(IntImage image, int magicNumber, const char *path) {
  ImageDomain domain = getIntImageDomain(image);
  int minX, maxX, minY, maxY, minVal, maxVal, width, height, npixels;
  width = getWidth(domain);
  height = getHeight(domain);
  npixels = width*height;
  getImageDomainValues(getIntImageDomain(image), &minX, &maxX, &minY, &maxY);
  getMinMax(image, &minVal, &maxVal);

  char *extension = getFileNameExtension(path);
  if (extension == NULL) {
    fatalError("saveIntImage: filename '%s' has no extension.\n", path);
  }

  if (strcmp(extension, "pgm") == 0) {
    if ((minVal < 0) || (maxVal > 65535)) {
      if (minVal < 0) minVal = 0;
      if (maxVal > 65535) maxVal = 65535;
      warning("saveIntImagePGM: image %s is clamped to [0,%d].\n", maxVal);
    }
    unsigned short *buffer = malloc(npixels*sizeof(unsigned short));
    int idx = 0;
    for (int y = minY; y <= maxY; y++) {
      for (int x = minX; x <= maxX; x++) {
        int val = getIntPixel(image, x, y);
        val = (val < 0 ? 0 : (val > 65535 ? 65535 : val));
        buffer[idx++] = val;
      }
    }
    if (magicNumber == 5) {
      saveImagePGMasP5(path, width, height, buffer);
    } else {
      saveImagePGMasP2(path, width, height, buffer);
    }
    free(buffer);
  }
}

static void saveIntImagePBM(IntImage image, int magicNumber, const char *path) {
  ImageDomain domain = getIntImageDomain(image);
  int minX, maxX, minY, maxY, minVal, maxVal, width, height, npixels;
  width = getWidth(domain);
  height = getHeight(domain);
  npixels = width*height;
  getImageDomainValues(getIntImageDomain(image), &minX, &maxX, &minY, &maxY);
  getMinMax(image, &minVal, &maxVal);

  if ((minVal < 0) || (maxVal > 1)) {
    if (minVal < 0) minVal = 0;
    if (maxVal > 1) maxVal = 1;
    warning("saveIntImagePBM: image %s is clamped to [0,%d].\n", path, maxVal);
  }
  uint8_t *buffer = malloc(npixels*sizeof(uint8_t));

  int idx = 0;
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      int val = getIntPixel(image, x, y);
      buffer[idx++] = (val < 0 ? 0 : (val == 0 ? 0 : 1));
    }
  }
  if (magicNumber == 1) {
    saveImagePBMasP1(path, width, height, buffer);
  } else {
    saveImagePBMasP4(path, width, height, buffer);
  }
  free(buffer);
}

void saveIntImagePGMRaw(IntImage image, const char *path) { saveIntImagePGM(image, 5, path); }

void saveIntImagePGMAscii(IntImage image, const char *path) { saveIntImagePGM(image, 2, path); }

void saveIntImagePBMAscii(IntImage image, const char *path) { saveIntImagePBM(image, 1, path); }

void saveIntImagePBMRaw(IntImage image, const char *path) { saveIntImagePBM(image, 4, path); }

void saveIntImage(IntImage image, const char *path) {
  char *extension = getFileNameExtension(path);
  if (extension == NULL) {
    fatalError("saveIntImage: filename '%s' has no extension.\n", path);
  }
  if (strcmp(extension, "pgm") == 0) {
    saveIntImagePGM(image, 5, path);
  }
  if (strcmp(extension, "pbm") == 0) {
    saveIntImagePBM(image, 4, path);
  }
}

/** Histogram ********************************************/

Histogram createHistogram(IntImage image) {
  int minRange, maxRange;
  getDynamicRange(image, &minRange, &maxRange);
  int minX, maxX, minY, maxY;
  getImageDomainValues(getIntImageDomain(image), &minX, &maxX, &minY, &maxY);

  int histSize = maxRange - minRange;
  int *frequencies = safeCalloc(histSize*sizeof(int));
  Histogram histogram;
  histogram.frequencies = frequencies;
  histogram.minRange = minRange;
  histogram.maxRange = maxRange;

  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      int val = getIntPixel(image, x, y);
      incrementHistogramFrequency(&histogram, val);
    }
  }
  return histogram;
}

void createRgbHistograms(RgbImage image, Histogram *redHist, Histogram *greenHist, Histogram *blueHist) {
  int minRange, maxRange;
  getRgbDynamicRange(image, &minRange, &maxRange);
  int minX, maxX, minY, maxY;
  getImageDomainValues(getRgbImageDomain(image), &minX, &maxX, &minY, &maxY);

  int histSize = maxRange - minRange;
  int *redFrequencies = safeCalloc(histSize*sizeof(int));
  int *greenFrequencies = safeCalloc(histSize*sizeof(int));
  int *blueFrequencies = safeCalloc(histSize*sizeof(int));
  redHist->frequencies = redFrequencies;
  redHist->minRange = minRange;
  redHist->maxRange = maxRange;

  greenHist->frequencies = greenFrequencies;
  greenHist->minRange = minRange;
  greenHist->maxRange = maxRange;

  blueHist->frequencies = blueFrequencies;
  blueHist->minRange = minRange;
  blueHist->maxRange = maxRange;
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      int r, g, b;
      getRgbPixel(image, x, y, &r, &g, &b);
      incrementHistogramFrequency(redHist, r);
      incrementHistogramFrequency(greenHist, g);
      incrementHistogramFrequency(blueHist, b);
    }
  }
}

Histogram createEmptyHistogram(int minRange, int maxRange) {
  int histSize = maxRange - minRange;
  int *frequencies = safeCalloc(histSize*sizeof(int));
  Histogram histogram;
  histogram.frequencies = frequencies;
  histogram.minRange = minRange;
  histogram.maxRange = maxRange;
  return histogram;
}

void freeHistogram(Histogram histogram) { free(histogram.frequencies); }

void getHistogramRange(Histogram histogram, int *minRange, int *maxRange) {
  *minRange = histogram.minRange;
  *maxRange = histogram.maxRange;
}

int getHistogramFrequency(Histogram histogram, int x) {
  if (x < histogram.minRange || x > histogram.maxRange) {
    fatalError("Attempt to access frequency for %d, which is outside the histogram domain [%d..%d].\n", x,
               histogram.minRange, histogram.maxRange);
  }
  return histogram.frequencies[x - histogram.minRange];
}

void setHistogramFrequency(Histogram *histogram, int x, int val) {
  if (x < histogram->minRange || x > histogram->maxRange) {
    fatalError("Attempt to access frequency for %d, which is outside the histogram domain [%d..%d].\n", x,
               histogram->minRange, histogram->maxRange);
  }
  histogram->frequencies[x - histogram->minRange] = val;
}

void incrementHistogramFrequency(Histogram *histogram, int x) {
  if (x < histogram->minRange || x > histogram->maxRange) {
    fatalError("Attempt to access frequency for %d, which is outside the histogram domain [%d..%d].\n", x,
               histogram->minRange, histogram->maxRange);
  }
  histogram->frequencies[x - histogram->minRange]++;
}

void printHistogram(Histogram histogram) {
  for (int i = histogram.minRange; i < histogram.maxRange; i++) {
    printf("%d:%d  ", i, getHistogramFrequency(histogram, i));
  }
  printf("\n");
}

//-------------------------------------------------------------------------

RgbImage allocateRgbImage(int width, int height, int minValue, int maxValue) {
  return allocateRgbImageGrid(0, width - 1, 0, height - 1, minValue, maxValue);
}

RgbImage allocateFromRgbImage(RgbImage image) {
  ImageDomain domain = getRgbImageDomain(image);
  int minX, maxX, minY, maxY;
  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);
  int minRange, maxRange;
  getRgbDynamicRange(image, &minRange, &maxRange);
  RgbImage image2 = allocateRgbImageGrid(minX, maxX, minY, maxY, minRange, maxRange);
  return image2;
}

RgbImage allocateRgbImageGrid(int minX, int maxX, int minY, int maxY, int minValue, int maxValue) {
  RgbImage image;
  ImageDomain domain = initImageDomain(minX, maxX, minY, maxY);
  image.domain = domain;
  image.red = allocIntMatrix(getWidth(domain), getHeight(domain));
  image.green = allocIntMatrix(getWidth(domain), getHeight(domain));
  image.blue = allocIntMatrix(getWidth(domain), getHeight(domain));
  // Dynamic range rgb values
  image.minRange = minValue;
  image.maxRange = maxValue;
  return image;
}

RgbImage allocateDefaultRgbImage(int width, int height) { return allocateRgbImage(width, height, INT_MIN, INT_MAX); }

RgbImage copyRgbImage(RgbImage image) {
  ImageDomain domain = getRgbImageDomain(image);
  int minX, maxX, minY, maxY;
  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);
  RgbImage copy = allocateFromRgbImage(image);
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      int r, g, b;
      getRgbPixel(image, x, y, &r, &g, &b);
      setRgbPixel(&copy, x, y, r, g, b);
    }
  }
  return copy;
}

void freeRgbImage(RgbImage image) {
  free(image.red);
  free(image.green);
  free(image.blue);
}

ImageDomain getRgbImageDomain(RgbImage image) { return image.domain; }

void getRgbDynamicRange(RgbImage image, int *minRange, int *maxRange) {
  *minRange = image.minRange;
  *maxRange = image.maxRange;
}

void getRgbPixel(RgbImage image, int x, int y, int *r, int *g, int *b) {
#if FAST
  x -= image.domain.minX;
  y -= image.domain.minY;
  *r = image.red[y][x];
  *g = image.green[y][x];
  *b = image.blue[y][x];
#else
  int minX, maxX, minY, maxY;
  getImageDomainValues(image.domain, &minX, &maxX, &minY, &maxY);
  checkDomain(x, y, minX, maxX, minY, maxY);
  x -= minX;
  y -= minY;
  *r = image.red[y][x];
  *g = image.green[y][x];
  *b = image.blue[y][x];

#endif
}

static int clampPixelValue(int val, int minRange, int maxRange) {
  if (val < minRange) {
    warning("setRgbPixel: value %d is outside dynamic range [%d,%d]: clamped to %d\n", val, minRange, maxRange,
            minRange);
    return minRange;
  }
  if (val > maxRange) {
    warning("setRgbPixel: value %d is outside dynamic range [%d,%d]: clamped to %d\n", val, minRange, maxRange,
            maxRange);
    return maxRange - 1;
  }
  return val;
}

/* ----------------------------- Image Setters ----------------------------- */

void setRgbPixel(RgbImage *image, int x, int y, int r, int g, int b) {
#if FAST
  x -= image->domain.minX;
  y -= image->domain.minY;
  image->red[y][x] = r;
  image->green[y][x] = g;
  image->blue[y][x] = b;
#else
  r = clampPixelValue(r, image->minRange, image->maxRange);
  g = clampPixelValue(g, image->minRange, image->maxRange);
  b = clampPixelValue(b, image->minRange, image->maxRange);
  int minX, maxX, minY, maxY;
  getImageDomainValues(image->domain, &minX, &maxX, &minY, &maxY);
  checkDomain(x, y, minX, maxX, minY, maxY);

  x -= minX;
  y -= minY;
  image->red[y][x] = r;
  image->green[y][x] = g;
  image->blue[y][x] = b;
#endif
}

void setAllRgbPixels(RgbImage *image, int r, int g, int b) {
  // prevent print spam
  r = clampPixelValue(r, image->minRange, image->maxRange);
  g = clampPixelValue(g, image->minRange, image->maxRange);
  b = clampPixelValue(b, image->minRange, image->maxRange);
  int minX, maxX, minY, maxY;
  getImageDomainValues(getRgbImageDomain(*image), &minX, &maxX, &minY, &maxY);
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      setRgbPixel(image, x, y, r, g, b);
    }
  }
}

/* ----------------------------- Image Printing + Viewing ----------------------------- */

void printRgbBuffer(RgbImage image) {
  int minX, maxX, minY, maxY;
  getImageDomainValues(getRgbImageDomain(image), &minX, &maxX, &minY, &maxY);
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      int r, g, b;
      getRgbPixel(image, x, y, &r, &g, &b);
      printf("(%d,%d,%d) ", r, g, b);
    }
    printf("\n");
  }
}

void printRgbImageLatexTable(RgbImage image) { printRgbLatexTableToFile(stdout, image); }

void printRgbLatexTableToFile(FILE *out, RgbImage image) {
  int minX, maxX, minY, maxY;
  getImageDomainValues(getRgbImageDomain(image), &minX, &maxX, &minY, &maxY);
  fprintf(out, "\\begin{tabular}{|c|");
  for (int x = minX; x <= maxX; x++) {
    fprintf(out, "|c");
  }
  fprintf(out, "|}\n\\hline\n(x,y)");
  for (int x = minX; x <= maxX; x++) {
    fprintf(out, "&%d", x);
  }
  fprintf(out, "\\\\\n\\hline\n");
  fprintf(out, "\\hline\n");
  for (int y = minY; y <= maxY; y++) {
    fprintf(out, "%d", y);
    for (int x = minX; x <= maxX; x++) {
      int r, g, b;
      getRgbPixel(image, x, y, &r, &g, &b);
      if ((y == 0) && (x == 0)) {  // origin
        fprintf(out, "&{\\bf (%d,%d,%d)}", r, g, b);
      } else {
        fprintf(out, "& (%d,%d,%d)", r, g, b);
      }
    }
    fprintf(out, "\\\\\\hline\n");
  }
  fprintf(out, "\\end{tabular}\n");
}

void displayRgbImage(RgbImage image, const char *windowTitle) {
  uint8_t *rBuf, *gBuf, *bBuf;
  rgbImageToByteBuffers(image, &rBuf, &gBuf, &bBuf);
  ImageDomain domain = getRgbImageDomain(image);
  int minX, maxX, minY, maxY, width = getWidth(domain), height = getHeight(domain);
  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);
  glutRgbViewer(rBuf, gBuf, bBuf, width, height, windowTitle);
}

/* ----------------------------- Image Loading + Saving ----------------------------- */

static unsigned short *loadImagePPM(const char *path, int *width, int *height, int *maxVal) {
  int magicNumber;
  FILE *imgFile = fopen(path, "r");
  if (fscanf(imgFile, "P%d\n", &magicNumber) != 1) {
    fclose(imgFile);
    fatalError("loadPgmImage: corrupt PGM: no magic number found.\n");
  }

  if ((magicNumber != 3) && (magicNumber != 6)) {
    fclose(imgFile);
    fatalError("Illegal magic number P%d found. Only P3 and P6 are valid PPM files.\n", magicNumber);
  }
  // skip comment lines
  char c;
  while ((c = fgetc(imgFile)) == '#') {
    do {
      c = fgetc(imgFile);
    } while ((c != EOF) && (c != '\n'));
    if (c == EOF) {
      fatalError("loadImagePPM: corrupt PPM file.\n");
    }
  }
  ungetc(c, imgFile);

  // read width and height
  if (fscanf(imgFile, "%d %d", width, height) != 2) {
    fatalError("loadImagePPM: corrupt PPM: no file dimensions found.\n");
  }
  if (fscanf(imgFile, "%d\n", maxVal) != 1) {
    fatalError("loadImagePPM: corrupt PPM file: no maximal grey value.\n");
  }
  if ((*maxVal < 0) || (*maxVal > 65535)) {
    fatalError("loadImagePPM: corrupt PPM: maximum value found is %d (must be in range [0..65535]).\n");
  }

  int npixels = (*width)*(*height);
  unsigned short *buffer = malloc(3*npixels*sizeof(short));
  if (magicNumber == 3) {
    int gval;
    for (int i = 0; i < 3*npixels; i++) {
      if (fscanf(imgFile, "%d", &gval) != 1) {
        fatalError("loadImagePPM: corrupt PPM: non numeric data found in PPM image (P3 type).\n");
      }
      if ((gval < 0) || (gval > *maxVal)) {
        fatalError("loadImagePPM: pixel with value %d found. Valid dynamic range is [0..%d].\n", gval, *maxVal);
      }
      buffer[i] = gval;
    }
  } else {
    // magicnumber == 6
    if (*maxVal > 255) {  // 2 bytes per pixel, i.e. short
      if (fread(buffer, 2, 3*npixels, imgFile) != 3*npixels) {
        fatalError("loadImagePPM: corrupt PPM, file is truncated.\n");
      }
    } else {  // 1 byte per pixel
      unsigned char *buf = malloc(3*npixels*sizeof(unsigned char));
      if (fread(buf, 1, 3*npixels, imgFile) != 3*npixels) {
        fatalError("loadImagePPM: corrupt PPM, file is truncated.\n");
      }
      for (int i = 0; i < 3*npixels; i++) {
        buffer[i] = buf[i];
      }
      free(buf);
    }
  }
  fclose(imgFile);
  return buffer;
}

RgbImage loadRgbImage(const char *path) {
  char *extension = getFileNameExtension(path);
  if (extension == NULL) {
    fatalError("loadIntImage: filename '%s' has no extension.\n", path);
  }
  FILE *f = fopen(path, "r");
  if (f == NULL) {
    fatalError("loadIntImage: failed to open file '%s'.\n", path);
  }
  fclose(f);

  int width, height, maxVal;
  unsigned short *buf = NULL;

  if (strcmp("ppm", extension) == 0) {
    buf = loadImagePPM(path, &width, &height, &maxVal);
  }
  // copy buffer into image structure
  RgbImage image = allocateRgbImage(width, height, 0, maxVal);
  int idx = 0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int r = buf[idx++];
      int g = buf[idx++];
      int b = buf[idx++];
      setRgbPixel(&image, x, y, r, g, b);
    }
  }
  free(buf);
  return image;
}

static void getRgbMinMax(RgbImage image, int *minimalValue, int *maximalValue) {
  int minX, maxX, minY, maxY, minVal, maxVal;
  getImageDomainValues(getRgbImageDomain(image), &minX, &maxX, &minY, &maxY);
  int r, g, b;
  getRgbPixel(image, minX, minY, &r, &g, &b);
  minVal = maxVal = r;
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      getRgbPixel(image, x, y, &r, &g, &b);
      minVal = (r < minVal ? r : minVal);
      maxVal = (r > maxVal ? r : maxVal);
      minVal = (g < minVal ? g : minVal);
      maxVal = (g > maxVal ? g : maxVal);
      minVal = (b < minVal ? b : minVal);
      maxVal = (b > maxVal ? b : maxVal);
    }
  }
  *minimalValue = minVal;
  *maximalValue = maxVal;
}

static void saveImagePPMasP6(const char *path, int width, int height, unsigned short *buffer) {
  FILE *ppmFile = fopen(path, "w");
  if (ppmFile == NULL) {
    fatalError("saveImagePPMasP6: failed to open file '%s'.\n", path);
  }
  fprintf(ppmFile, "P6\n%d %d\n", width, height);

  int npixels = (width)*(height);
  unsigned short max = buffer[0];
  for (int i = 0; (i < 3*npixels) && (max < 65535); i++) {
    max = (buffer[i] > max ? buffer[i] : max);
  }
  fprintf(ppmFile, "%d\n", max);
  if (max > 255) {
    fwrite(buffer, 2, 3*npixels, ppmFile);
  } else {
    unsigned char *buf = malloc(3*npixels);
    for (int i = 0; i < 3*npixels; i++) {
      buf[i] = buffer[i];
    }
    fwrite(buf, 1, 3*npixels, ppmFile);
    free(buf);
  }
  fclose(ppmFile);
}

static void saveImagePPMasP3(const char *path, int width, int height, unsigned short *buffer) {
  FILE *ppmFile = fopen(path, "w");
  if (ppmFile == NULL) {
    fatalError("saveImagePGMasP2: failed to open file '%s'.\n", path);
  }
  fprintf(ppmFile, "P3\n%d %d\n", width, height);

  int npixels = (width)*(height);
  unsigned short max = buffer[0];
  for (int i = 0; (i < 3*npixels) && (max < 65535); i++) {
    max = (buffer[i] > max ? buffer[i] : max);
  }
  fprintf(ppmFile, "%d\n", max);
  int idx = 0;
  for (int i = 0; i < height; i++) {
    for (int c = 0; c < 3; i++) {
      fprintf(ppmFile, "%d", buffer[idx++]);
    }
    for (int j = 1; j < width; j++) {
      for (int c = 0; c < 3; i++) {
        fprintf(ppmFile, " %d", buffer[idx++]);
      }
    }
    fprintf(ppmFile, "\n");
  }
  fclose(ppmFile);
}

static void saveRgbImagePPM(RgbImage image, int magicNumber, const char *path) {
  ImageDomain domain = getRgbImageDomain(image);
  int minX, maxX, minY, maxY, minVal, maxVal, width, height, npixels;
  width = getWidth(domain);
  height = getHeight(domain);
  npixels = width*height;
  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);
  getRgbMinMax(image, &minVal, &maxVal);

  char *extension = getFileNameExtension(path);
  if (extension == NULL) {
    fatalError("saveRgbImagePPM: filename '%s' has no extension.\n", path);
  }

  if (strcmp(extension, "ppm") == 0) {
    if ((minVal < 0) || (maxVal > 65535)) {
      if (minVal < 0) minVal = 0;
      if (maxVal > 65535) maxVal = 65535;
      warning("saveRgbImagePPM: image %s is clamped to [0,%d].\n", maxVal);
    }
    unsigned short *buffer = malloc(3*npixels*sizeof(unsigned short));
    int idx = 0;
    for (int y = minY; y <= maxY; y++) {
      for (int x = minX; x <= maxX; x++) {
        int r, g, b;
        getRgbPixel(image, x, y, &r, &g, &b);
        r = (r < 0 ? 0 : (r > 65535 ? 65535 : r));
        g = (g < 0 ? 0 : (g > 65535 ? 65535 : g));
        b = (b < 0 ? 0 : (b > 65535 ? 65535 : b));
        buffer[idx++] = r;
        buffer[idx++] = g;
        buffer[idx++] = b;
      }
    }
    if (magicNumber == 6) {
      saveImagePPMasP6(path, width, height, buffer);
    } else {
      saveImagePPMasP3(path, width, height, buffer);
    }
    free(buffer);
  }
}

void saveRgbImage(RgbImage image, const char *path) {
  char *extension = getFileNameExtension(path);
  if (extension == NULL) {
    fatalError("saveRgbImage: filename '%s' has no extension.\n", path);
  }
  if (strcmp(extension, "ppm") == 0) {
    saveRgbImagePPM(image, 6, path);
  } else {
    fatalError("saveRgbImage: filename '%s' is not a ppm file.\n", path);
  }
}

void saveRgbImagePPMRaw(RgbImage image, const char *path) { saveRgbImagePPM(image, 6, path); }

void saveRgbImagePPMAscii(RgbImage image, const char *path) { saveRgbImagePPM(image, 3, path); }

/* ----------------------------- Image Operations ----------------------------- */

static RgbImage applyFunctionRgbImage(RgbImage imageA, RgbImage imageB, binaryOp operator) {
  RgbImage result = allocateFromRgbImage(imageA);
  int minX, maxX, minY, maxY;
  getImageDomainValues(getRgbImageDomain(imageA), &minX, &maxX, &minY, &maxY);
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      int r1, g1, b1;
      getRgbPixel(imageA, x, y, &r1, &g1, &b1);
      int r2, g2, b2;
      getRgbPixel(imageB, x, y, &r2, &g2, &b2);
      setRgbPixel(&result, x, y, operator(r1, r2), operator(g1, g2), operator(b1, b2));
    }
  }
  return result;
}

static void compareRgbDomains(RgbImage imageA, RgbImage imageB) {
  int minX1, maxX1, minY1, maxY1;
  getImageDomainValues(getRgbImageDomain(imageA), &minX1, &maxX1, &minY1, &maxY1);
  int minX2, maxX2, minY2, maxY2;
  getImageDomainValues(getRgbImageDomain(imageA), &minX2, &maxX2, &minY2, &maxY2);
  if (minX1 != minX2 || maxX1 != maxX2 || minY1 != minY2 || maxY1 != maxY2) {
    fatalError("Images do not have the same domain.");
  }
}

RgbImage maxRgbImage(RgbImage imageA, RgbImage imageB) {
  compareRgbDomains(imageA, imageB);
  return applyFunctionRgbImage(imageA, imageB, &maxOp);
}

RgbImage minRgbImage(RgbImage imageA, RgbImage imageB) {
  compareRgbDomains(imageA, imageB);
  return applyFunctionRgbImage(imageA, imageB, &minOp);
}

RgbImage addRgbImage(RgbImage imageA, RgbImage imageB) {
  compareRgbDomains(imageA, imageB);
  return applyFunctionRgbImage(imageA, imageB, &addOp);
}

RgbImage subtractRgbImage(RgbImage imageA, RgbImage imageB) {
  compareRgbDomains(imageA, imageB);
  return applyFunctionRgbImage(imageA, imageB, &subtractOp);
}

RgbImage multiplyRgbImage(RgbImage imageA, RgbImage imageB) {
  compareRgbDomains(imageA, imageB);
  return applyFunctionRgbImage(imageA, imageB, &multiplyOp);
}

RgbImage applyLutRgbImage(RgbImage image, int **LUT, int LUTsize) {
  if (image.minRange < 0) {
    fatalError("applyLutIntImage: LUTs can only be applied to image with positive ");
  }
  if (image.maxRange > LUTsize) {
    fatalError("applyLutIntImage: LUT must be the same size as the dynamic range of the image");
  }

  RgbImage resultImg = allocateFromRgbImage(image);
  int minX, maxX, minY, maxY;
  getImageDomainValues(getRgbImageDomain(image), &minX, &maxX, &minY, &maxY);
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      int r, g, b;
      getRgbPixel(image, x, y, &r, &g, &b);
      setRgbPixel(&resultImg, x, y, LUT[r][0], LUT[g][1], LUT[b][2]);
    }
  }
  return resultImg;
}

/* ----------------------------- Image Transformations ----------------------------- */

RgbImage padRgbImage(RgbImage image, int top, int right, int bottom, int left, int r, int g, int b) {
  ImageDomain domain = getRgbImageDomain(image);
  ImageDomain paddedDomain = padDomain(domain, top, right, bottom, left);
  int minX, maxX, minY, maxY;
  getImageDomainValues(paddedDomain, &minX, &maxX, &minY, &maxY);
  int minRange, maxRange;
  getRgbDynamicRange(image, &minRange, &maxRange);
  RgbImage paddedImg = allocateRgbImageGrid(minX, maxX, minY, maxY, minRange, maxRange);

  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      if (isInDomain(domain, x, y)) {
        int imR, imG, imB;
        getRgbPixel(image, x, y, &imR, &imG, &imB);
        setRgbPixel(&paddedImg, x, y, imR, imG, imB);
      } else {
        setRgbPixel(&paddedImg, x, y, r, g, b);
      }
    }
  }
  return paddedImg;
}

void translateRgbImage(RgbImage *image, int x, int y) {
  ImageDomain *domain = getRgbImageDomainPtr(image);
  domain->minX += x;
  domain->maxX += x;
  domain->minY += y;
  domain->maxY += y;
}

void flipRgbImageHorizontal(RgbImage *image) {
  ImageDomain domain = getRgbImageDomain(*image);
  int width = getWidth(domain);
  int height = getHeight(domain);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x <= width/2; x++) {
      int a = image->red[y][x];
      image->red[y][x] = image->red[y][width - x - 1];
      image->red[y][width - x - 1] = a;
      a = image->green[y][x];
      image->green[y][x] = image->green[y][width - x - 1];
      image->green[y][width - x - 1] = a;
      a = image->blue[y][x];
      image->blue[y][x] = image->blue[y][width - x - 1];
      image->blue[y][width - x - 1] = a;
    }
  }
  flipDomainHorizontal(getRgbImageDomainPtr(image));
}

void flipRgbImageVertical(RgbImage *image) {
  ImageDomain domain = getRgbImageDomain(*image);
  int width = getWidth(domain);
  int height = getHeight(domain);
  for (int y = 0; y <= height/2; y++) {
    for (int x = 0; x < width; x++) {
      int a = image->red[y][x];
      image->red[y][x] = image->red[height - y - 1][x];
      image->red[height - y - 1][x] = a;
      a = image->green[y][x];
      image->green[y][x] = image->green[height - y - 1][x];
      image->green[height - y - 1][x] = a;
      a = image->blue[y][x];
      image->blue[y][x] = image->blue[height - y - 1][x];
      image->blue[height - y - 1][x] = a;
    }
  }
  flipDomainHorizontal(getRgbImageDomainPtr(image));
}

/* ----------------------------- Distance Transforms ----------------------------- */

static IntImage maskDistanceTransform(int maskLength, int *maskDx, int *maskDy, int foreground, IntImage im) {
  ImageDomain domain = getIntImageDomain(im);
  int width = getWidth(domain);
  int height = getHeight(domain);
  int infinity = width + height + 1;
  int minX, maxX, minY, maxY;
  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);
  IntImage dt = allocateIntImageGrid(minX, maxX, minY, maxY, 0, infinity);

  /* top-down, left-to-right pass */
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      if (getIntPixel(im, x, y) != foreground) {
        setIntPixel(&dt, x, y, 0);
      } else {
        int nb, minnb = infinity;
        for (int n = 0; n < maskLength; n++) {
          int nbx = x + maskDx[n];
          int nby = y + maskDy[n];
          if ((minX <= nbx) && (nbx <= maxX) && (minY <= nby) && (nby <= maxY)) {
            // this routine can be optimized quite a bit to make sure that
            // the above boundary checking is not needed for ech pixel.
            // feel free to optimize it.
            nb = getIntPixel(dt, nbx, nby);
            minnb = (minnb < nb ? minnb : nb);
          }
        }
        minnb = (minnb < infinity ? 1 + minnb : infinity);
        setIntPixel(&dt, x, y, minnb);
      }
    }
  }
  /* bottom-up, right-to-left pass */
  for (int y = maxY; y >= minY; y--) {
    for (int x = maxX; x >= minX; x--) {
      int here = getIntPixel(dt, x, y);
      if (here > 0) {
        int nb, minnb = infinity;
        for (int n = 0; n < maskLength; n++) {
          int nbx = x - maskDx[n];
          int nby = y - maskDy[n];
          if ((minX <= nbx) && (nbx <= maxX) && (minY <= nby) && (nby <= maxY)) {
            nb = getIntPixel(dt, nbx, nby);
            minnb = (minnb < nb ? minnb : nb);
          }
        }
        minnb = (minnb < infinity ? 1 + minnb : infinity);
        here = (minnb < here ? minnb : here);
        setIntPixel(&dt, x, y, here);
      }
    }
  }
  return dt;
}

IntImage dt4RosenfeldPfaltz(int foreground, IntImage im) {
  int maskDx[4] = {-1, 0};
  int maskDy[4] = {0, -1};
  return maskDistanceTransform(2, maskDx, maskDy, foreground, im);
}

IntImage dt8RosenfeldPfaltz(int foreground, IntImage im) {
  int maskDx[4] = {-1, 0, 1, -1};
  int maskDy[4] = {-1, -1, -1, 0};
  return maskDistanceTransform(4, maskDx, maskDy, foreground, im);
}

/** Euclidean distance transform ****************************************/

/* The Euclidean distance transform according to
*Arnold Meijster, Jos BTM Roerdink, and Wim H Hesselink.
*"A general algorithm for computing distance transforms
*in linear time.", In: Mathematical Morphology and its
*Applications to Image and Signal Processing, J. Goutsias,
*L. Vincent, and D.S. Bloomberg (eds.), Kluwer, 2000, pp. 331-340.
 */

/*********************** Distance dependant functions *********************/

static IntImage dtMeijsterRoerdinkHesselink(int takeSquareRoot, int foreground, IntImage im) {
  ImageDomain domain = getIntImageDomain(im);

  int width = getWidth(domain);
  int height = getHeight(domain);
  int minX, maxX, minY, maxY, infinity = width*width + height*height;  // or anything larger than this

  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);

  translateIntImage(&im, -minX, -minY);

  /* vertical phase */
  int maskDx[1] = {0};
  int maskDy[1] = {-1};
  IntImage verticalDT = maskDistanceTransform(2, maskDx, maskDy, foreground, im);

  /* square the verticalDT */
  IntImage vdt = allocateIntImage(width, height, 0, infinity);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int vdtval = getIntPixel(verticalDT, x, y);
      vdtval = (vdtval < height ? vdtval*vdtval : infinity);
      setIntPixel(&vdt, x, y, vdtval);
    }
  }
  freeIntImage(verticalDT);
  /* horizontal phase */
  IntImage dt = allocateIntImage(width, height, 0, infinity);
  int *s = malloc(width*sizeof(int));
  int *t = malloc(width*sizeof(int));
  int q;
  for (int y = 0; y < height; y++) {
    /* left-to-right scan */
    q = s[0] = t[0] = 0;
    for (int x = 1; x < width; x++) {
      int vsq = getIntPixel(vdt, s[q], y);
      int vxy = getIntPixel(vdt, x, y);
      while ((q >= 0) && ((t[q] - s[q])*(t[q] - s[q]) + vsq > (t[q] - x)*(t[q] - x) + vxy)) {
        q--;
        vsq = getIntPixel(vdt, s[q], y);
      }
      if (q < 0) {
        q = 0;
        s[0] = x;
      } else {
        int w = 1 + (x*x - s[q]*s[q] + vxy - vsq)/(2*(x - s[q]));
        if (w < width) {
          q++;
          s[q] = x;
          t[q] = w;
        }
      }
    }
    /* backward scan */
    int vsq = getIntPixel(vdt, s[q], y);
    if (takeSquareRoot) {
      for (int x = width - 1; x >= 0; x--) {
        setIntPixel(&dt, x, y, 0.5 + sqrt((x - s[q])*(x - s[q]) + vsq));
        if (x == t[q]) {
          q--;
          vsq = getIntPixel(vdt, s[q], y);
        }
      }
    } else {
      for (int x = width - 1; x >= 0; x--) {
        setIntPixel(&dt, x, y, (x - s[q])*(x - s[q]) + vsq);
        if (x == t[q]) {
          q--;
          vsq = getIntPixel(vdt, s[q], y);
        }
      }
    }
  }
  /* clean up */
  free(t);
  free(s);
  freeIntImage(vdt);

  translateIntImage(&dt, minX, minY);
  return dt;
}

IntImage distanceTransform(IntImage image, int metric, int foreground) {
  switch (metric) {
    case MANHATTAN:
      return dt4RosenfeldPfaltz(foreground, image);
    case CHESSBOARD:
      return dt8RosenfeldPfaltz(foreground, image);
    case EUCLID:
      return dtMeijsterRoerdinkHesselink(1, foreground, image);
    case SQEUCLID:
      return dtMeijsterRoerdinkHesselink(0, foreground, image);
  }
  fatalError(
      "distanceTransform: unrecognized metric value "
      "(must be MANHATTAN, CHESSBOARD, EUCLID, or SQEUCLID).\n");
  // won't be reached, but neccesary to stop compiler warning.
  exit(EXIT_FAILURE);
}