#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "improc.h"

void thresholdDemo(const char *path) {
  IntImage image = loadIntImage(path);
  ImageDomain domain = getIntImageDomain(image);
  displayIntImage(image, "Source Image");
  int minX, maxX, minY, maxY;
  getImageDomainValues(domain, &minX, &maxX, &minY, &maxY);
  IntImage thresholdedImage = allocateIntImage(getWidth(domain), getHeight(domain), 0, 255);
  for (int threshold = 64; threshold < 256; threshold += 64) {
    for (int y = minY; y <= maxY; y++) {
      for (int x = minX; x <= maxX; x++) {
        if (getIntPixel(image, x, y) < threshold) {
          setIntPixel(&thresholdedImage, x, y, 0);
        } else {
          setIntPixel(&thresholdedImage, x, y, 255);
        }
      }
    }
    char filename[20];
    sprintf(filename, "threshold%d.pbm", threshold);
    displayIntImage(thresholdedImage, filename);  // filename is used as window title
    saveIntImage(thresholdedImage, filename);
  }
  // Clean up
  freeIntImage(thresholdedImage);
  freeIntImage(image);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Fatal error: Please provide an image file as arugment.\n");
    exit(EXIT_FAILURE);
  }
  thresholdDemo(argv[1]);
  return EXIT_SUCCESS;
}
