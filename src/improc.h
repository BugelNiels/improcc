/**
 * @file improc.h
 * @author Arnold Meijster (a.meijster@rug.nl), Niels Bugel (n.a.bugel@rug.nl)
 * @brief This file contains all the functions you need to load, save, view and manipulate images. In an image, each
 * pixel is represented by a an integer. Each image has a certain dynamic range. This refers to a minimum and maximum
 * value a pixel in the image can have at most. Attempting to store a value that is not in the dynamic range of an image
 * is not possible.
 *
 * Whereas traditional images have an x domain of [0..width) and a y domain
 * of [0..height), the images in this framework have an x domain of [minX..maxX] and a y domain of [minY..maxY]. This
 * means that these images can work with negative indices if the image domain allows for it.
 * @version 1.0
 * @date 2022-02-21
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef IMPROC_H
#define IMPROC_H

// Distance transform metrics
#define MANHATTAN 0
#define CHESSBOARD 1
#define EUCLID 2
#define SQEUCLID 3

#include "stdio.h"

typedef struct ImageDomain {
  int minX, maxX;
  int minY, maxY;
} ImageDomain;

typedef struct IntImage {
  ImageDomain domain;
  int **pixels;
  int minRange, maxRange;
} IntImage;

typedef struct RgbImage {
  ImageDomain domain;
  int **red;
  int **green;
  int **blue;
  int minRange, maxRange;
} RgbImage;

typedef struct Histogram {
  int *frequencies;
  int minRange, maxRange;
} Histogram;

/* ----------------------------- Image Initialization ----------------------------- */

/**
 * @brief Allocates an empty image in the domain [0...width) x [0..height) with the specified parameters.
 *
 * @param width The width of the image in pixels.
 * @param height The height of the image in pixels.
 * @param minValue The minimum possible value this image should be able to contain.
 * @param maxValue The maximum possible value this image should be able to contain.
 * @return IntImage A newly allocated IntImage. Note that you should free the resulting image when you are done with it.
 */
IntImage allocateIntImage(int width, int height, int minValue, int maxValue);

/**
 * @brief Allocates an empty image in the domain [0...width) x [0..height) with an infinite dynamic range.
 *
 * @param width The width of the image in pixels.
 * @param height The height of the image in pixels.
 * @return IntImage A newly allocated IntImage. Note that you should free the resulting image when you are done with it.
 */
IntImage allocateDefaultIntImage(int width, int height);

/**
 * @brief Allocates an empty image with the domain and dynamic range of the provided image. Does not copy pixel values.
 *
 * @param image The image whose properties to copy.
 * @return IntImage A newly allocated IntImage.
 */
IntImage allocateFromIntImage(IntImage image);

/**
 * @brief Creates a copy of the provided image.
 *
 * @param image The image to copy
 * @return IntImage A copy of the provided image
 */
IntImage copyIntImage(IntImage image);

/**
 * @brief Allocates an empty image in the domain [minX...maxX] x [minY..maxY] with the specified parameters.
 *
 * @param minX The start of the image domain in the x direction.
 * @param maxX The end of the image domain in the x direction.
 * @param minY The start of the image domain in the y direction.
 * @param maxY The end of the image domain in the y direction.
 * @param minValue The minimum possible value this image should be able to contain.
 * @param maxValue The maximum possible value this image should be able to contain.
 * @return IntImage A newly allocated IntImage. Note that you should free the resulting image when you are done with it.
 */

IntImage allocateIntImageGrid(int minX, int maxX, int minY, int maxY, int minValue, int maxValue);

/**
 * @brief Frees the memory used by the provided image.
 *
 * @param image The image for which to free the memory.
 */
void freeIntImage(IntImage image);

/* ----------------------------- Image Getters ----------------------------- */

/**
 * @brief Retrieve the domain information of the provided image.
 *
 * @param image The image from which to retrieve the domain.
 * @return ImageDomain The domain of the image.
 */
ImageDomain getIntImageDomain(IntImage image);

// trivial domain getters/setters
int getMinX(ImageDomain domain);
int getMaxX(ImageDomain domain);
int getMinY(ImageDomain domain);
int getMaxY(ImageDomain domain);
int getWidth(ImageDomain domain);
int getHeight(ImageDomain domain);

/**
 * @brief Quality of life function. Puts the properties of the image domain into the provided arguments.
 *
 * @param domain The domain of the image
 * @param minX The start of the image domain in the x direction.
 * @param maxX The end of the image domain in the x direction.
 * @param minY The start of the image domain in the y direction.
 * @param maxY The end of the image domain in the y direction.
 */
void getImageDomainValues(ImageDomain domain, int *minX, int *maxX, int *minY, int *maxY);

/**
 * @brief Puts the minimum value and maximum value found in the provided image into the minimalValue and maximalValue
 * respectively.
 *
 * @param image The image in which to find the minimum and maximum values.
 * @param minimalValue The resulting minimal value will be put here.
 * @param maximalValue The resulting maximal value will be put here.
 */
void getMinMax(IntImage image, int *minimalValue, int *maximalValue);

/**
 * @brief Retrieve the dynamic range of the provided image.
 *
 * @param image The image from which to retrieve the dynamic range.
 * @param minRange The minimum possible value this image is able to contain will be put here.
 * @param maxRange The maximum possible value this image is able to contain will be put here.
 */
void getDynamicRange(IntImage image, int *minRange, int *maxRange);

/**
 * @brief Retrieves the pixel value of the image at the provided coordinates. Note that x and y can be negative if the
 * image domain allows for this.
 *
 * @param image The image from which to retrieve the pixel value.
 * @param x The x coordinate of the pixel to retrieve.
 * @param y The y coordinate of the pixel to retrieve.
 * @return int The grey value at (x,y).
 */
int getIntPixel(IntImage image, int x, int y);

/* ----------------------------- Image Setters ----------------------------- */

/**
 * @brief Set the grey value of the image at the provided coordinates. Note that x and y can be negative if the
 * image domain allows for this. Additionally, the grey value should fit in the dynamic range of the image.
 *
 * @param image The image in which to set the pixel value.
 * @param x The x coordinate of the pixel to set.
 * @param y The y coordinate of the pixel to set.
 * @param greyValue The grey value to put at (x,y).
 */
void setIntPixel(IntImage *image, int x, int y, int greyValue);

/**
 * @brief Sets all the pixels in the provided image to the provided grey value. Note that the grey value should fit in
 * the dynamic range of the image.
 *
 * @param image The image in which to set all the pixel values.
 * @param greyValue The grey value to put in the image.
 */
void setAllIntPixels(IntImage *image, int greyValue);

/**
 * @brief Updates the dynamic range of the image
 *
 * @param image The image to update the dynamic range of
 * @param newMinRange The new minimum possible value this image should be able to contain.
 * @param newMaxRange The new maximum possible value this image should be able to contain.
 */
void setDynamicRange(IntImage *image, int newMinRange, int newMaxRange);

/* ----------------------------- Image Printing + Viewing ----------------------------- */

/**
 * @brief Prints all the pixel values in the provided image to stdout. Every row is put on a new line.
 *
 * @param image The image to print.
 */
void printIntBuffer(IntImage image);

/**
 * @brief Prints a LaTeX compatible table representation of the provided image to stdout.
 *
 * @param image The image to print the LaTeX table of.
 */
void printIntImageLatexTable(IntImage image);

/**
 * @brief Prints a LaTeX compatible table representation of the provided image to the provided file stream.
 *
 * @param out File stream to print the image to.
 * @param image The image to print the LaTeX table of.
 */
void printIntLatexTableToFile(FILE *out, IntImage image);

/**
 * @brief Opens a window that allows the user to view the image. Note that this uses OpenGL which in turn allocates
 * memory that cannot be freed. If you want to check for memory leaks, make sure that you do not run any image displays.
 *
 * @param image The image to view
 * @param windowTitle The title of the window.
 */
void displayIntImage(IntImage image, const char *windowTitle);

/* ----------------------------- Image Loading + Saving ----------------------------- */

/**
 * @brief Loads an image from the provided file. Supported extensions: .pbm, .pgm, .ppm.
 *
 * @param path The path of the image to load.
 * @return IntImage An integer image represtation of the provided file.
 */
IntImage loadIntImage(const char *path);

/**
 * @brief Saves an image at the provided location. Supported extensions: .pbm, .pgm, .ppm. This will save the netpbm
 * files as their binary formats.
 *
 * @param image The images to save.
 * @param path The location to save the image at.
 */
void saveIntImage(IntImage image, const char *path);

/**
 * @brief Saves an image at the provided location. The location must be a .pgm file. Pixels values are stored as raw
 * bytes in the file.
 *
 * @param image The images to save.
 * @param path The location to save the image at.
 */
void saveIntImagePGMRaw(IntImage image, const char *path);

/**
 * @brief Saves an image at the provided location. The location must be a .pgm file. Pixels values are stored as ascii
 * (human readable) values in the file.
 *
 * @param image The images to save.
 * @param path The location to save the image at.
 */
void saveIntImagePGMAscii(IntImage image, const char *path);

/**
 * @brief Saves an image as a binary image at the provided location. The location must be a .pbm file. Pixels values are
 * stored as raw bits in the file.
 *
 * @param image The images to save.
 * @param path The location to save the image at.
 */
void saveIntImagePBMRaw(IntImage image, const char *path);

/**
 * @brief Saves an image as a binary image at the provided location. The location must be a .pbm file. Pixels values are
 * stored as ascii (human readable) values in the file.
 *
 * @param image The images to save.
 * @param path The location to save the image at.
 */
void saveIntImagePBMAscii(IntImage image, const char *path);

/* ----------------------------- Image Useful stuff ----------------------------- */

/**
 * @brief Creates a new image h from two input images f and g where each pixel is defined as h(x,y) = max(f(x,y),
 * g(x,y))
 *
 * @param imageA First input image
 * @param imageB Second input image
 * @return IntImage Output image where each pixel is the maximum of the corresponding pixels in the two input images
 */
IntImage maxIntImage(IntImage imageA, IntImage imageB);

/**
 * @brief Creates a new image h from two input images f and g where each pixel is defined as h(x,y) = min(f(x,y),
 * g(x,y))
 *
 * @param imageA First input image
 * @param imageB Second input image
 * @return IntImage Output image where each pixel is the minimum of the corresponding pixels in the two input images
 */
IntImage minIntImage(IntImage imageA, IntImage imageB);

/**
 * @brief Creates a new image h from two input images f and g where each pixel is defined as h(x,y) = f(x,y) + g(x,y)
 *
 * @param imageA First input image
 * @param imageB Second input image
 * @return IntImage Output image where each pixel is the sum of the corresponding pixels in the two input images
 */
IntImage addIntImage(IntImage imageA, IntImage imageB);

/**
 * @brief Creates a new image h from two input images f and g where each pixel is defined as h(x,y) = f(x,y) - g(x,y)
 *
 * @param imageA First input image
 * @param imageB Second input image
 * @return IntImage Output image where each pixel is the subtraction of the corresponding pixels in the two input images
 */
IntImage subtractIntImage(IntImage imageA, IntImage imageB);

/**
 * @brief Creates a new image h from two input images f and g where each pixel is defined as h(x,y) = f(x,y) * g(x,y)
 *
 * @param imageA First input image
 * @param imageB Second input image
 * @return IntImage Output image where each pixel is the multiplication of the corresponding pixels in the two input
 * images
 */
IntImage multiplyIntImage(IntImage imageA, IntImage imageB);

/**
 * @brief Produces an output image that is the result of applying a lookup table (LUT) to the input image. The LUT
 * should have the same size as the dynamic range of the input image.
 *
 * @param image Input image
 * @param LUT The lookup table. Maps a gray value in [minRange..maxRange] of the image to a new gray value.
 * @param LUTSize The size of the lookup table.
 * @return IntImage Image produced from applying the LUT to the input iamge
 */
IntImage applyLutIntImage(IntImage image, int *LUT, int LUTsize);

/* ----------------------------- Image Transformations ----------------------------- */

/**
 * @brief Performs a distance transform on the provided image.
 * It calculates for each foreground pixel the distance to the nearest background pixel based on some metric.
 *
 * @param image The image to perform the distance transform on.
 * @param metric The metric to use for the distance transform. Should be one of the constants: MANHATTAN, CHESSBOARD,
 * EUCLID or SQEUCLID.
 * @param foreground Pixels with this pixel value are assumed as foreground pixels in the distance transforms. Pixels
 * that have different values are assumed to be background pixels.
 * @return IntImage An image containing the distances of each pixel to the corresponding provided pixel value.
 */
IntImage distanceTransform(IntImage image, int metric, int foreground);

/**
 * @brief Produces a new, padded image from the provided image.  Note that this allocates a new image, which should
 * subsequently be freed.
 *
 * @param image The image to pad.
 * @param top Number of pixels to pad at the top.
 * @param right Number of pixels to pad to the right.
 * @param bottom Number of pixels to pad at the bottom.
 * @param left Number of pixels to pad to the left.
 * @param padValue Grey value the padded pixels should have.
 * @return IntImage The newly padded image.
 */
IntImage padIntImage(IntImage image, int top, int right, int bottom, int left, int padValue);

/**
 * @brief Translates the image domain by x, y. Does not affect the underlying pixel values, only the domain of the
 * image.
 *
 * @param image The image to translate.
 * @param x Number of pixels to translate in the x direction.
 * @param y Number of pixels to translate in the y direction.
 */
void translateIntImage(IntImage *image, int x, int y);

/**
 * @brief Horizontally flips an image around the origin.
 *
 * @param image The image to flip.
 */
void flipIntImageHorizontal(IntImage *image);

/**
 * @brief Vertically flips an image around the origin.
 *
 * @param image The image to flip.
 */
void flipIntImageVertical(IntImage *image);

/* ----------------------------- Image Histogram Functions ----------------------------- */

/**
 * @brief Creates a histogram from the provided image. Note that this will allocate a lot of memory when the dynamic
 * range is large, since each bin of the histogram is a single pixel value.
 *
 * @param image The image to create the histogram of.
 * @return Histogram The histogram of the image.
 */
Histogram createHistogram(IntImage image);

/**
 * @brief Creates a histogram for the channels red, green and blue from the provided image. Note that this will allocate
 * a lot of memory when the dynamic range is large, since each bin of the histogram is a single pixel value.
 *
 * @param image
 * @param redHist
 * @param greenHist
 * @param blueHist
 */
void createRgbHistograms(RgbImage image, Histogram *redHist, Histogram *greenHist, Histogram *blueHist);

/**
 * @brief Creates an empty histogram
 *
 * @param minRange Minimum possible pixel value in the histogram.
 * @param maxRange Maximum possible pixel value in the histogram.
 * @return Histogram Empy histogram with the value 0 for each pixel.
 */
Histogram createEmptyHistogram(int minRange, int maxRange);

/**
 * @brief Frees the memory used by the provided histogram.
 *
 * @param histogram The histogram for which to free the memory.
 */
void freeHistogram(Histogram histogram);

/**
 * @brief Retrieves the frequency of the provided pixel value in the histogram.
 *
 * @param histogram The histogram.
 * @param pixelVal The pixelvalue for which to retrieve the frequency.
 * @return int Frequency of the pixel value.
 */
int getHistogramFrequency(Histogram histogram, int pixelVal);

/**
 * @brief Sets the frequency of the provided pixel value to a certain value.
 *
 * @param histogram The histogram.
 * @param pixelVal The pixel value for which to set the frequency.
 * @param freq The frequency the provided pixel value should have.
 */
void setHistogramFrequency(Histogram *histogram, int pixelVal, int freq);

/**
 * @brief Increments the freqyency of the provided pixel value by 1.
 *
 * @param histogram The histogram.
 * @param pixelVal The pixel value to increment.
 */
void incrementHistogramFrequency(Histogram *histogram, int pixelVal);

/**
 * @brief Prints the histogram pixel value-frequency pairs.
 *
 * @param histogram The histogram to print.
 */
void printHistogram(Histogram histogram);

/**
 * @brief Retrieves the range of values the provided histogram contains.
 *
 * @param histogram The histogram to retrieve the dynamic range of.
 * @param minRange The value of the minimum bin in the histogram will be put here.
 * @param maxRange The value of the maximum bin in the histogram will be put here.
 */
void getHistogramRange(Histogram histogram, int *minRange, int *maxRange);

/* ----------------------------- Image Initialization ----------------------------- */

/**
 * @brief Allocates an empty image in the domain [0...width) x [0..height) with the specified parameters.
 *
 * @param width The width of the image in pixels.
 * @param height The height of the image in pixels.
 * @param minValue The minimum possible value a channel in this image should be able to contain.
 * @param maxValue The maximum possible value a channel in this image should be able to contain.
 * @return RgbImage A newly allocated RgbImage. Note that you should free the resulting image when you are done with it.
 */
RgbImage allocateRgbImage(int width, int height, int minValue, int maxValue);

/**
 * @brief Allocates an empty image in the domain [0...width) x [0..height) with an infinite dynamic range.
 *
 * @param width The width of the image in pixels.
 * @param height The height of the image in pixels.
 * @return RgbImage A newly allocated IntImage. Note that you should free the resulting image when you are done with it.
 */
RgbImage allocateDefaultRgbImage(int width, int height);

/**
 * @brief Allocates an empty image with the domain and dynamic range of the provided image. Does not copy pixel values.
 *
 * @param image The image whose properties to copy.
 * @return RgbImage A newly allocated RgbImage.
 */
RgbImage allocateFromRgbImage(RgbImage image);

/**
 * @brief Creates a copy of the provided image.
 *
 * @param image The image to copy.
 * @return RgbImage A copy of the provided image.
 */
RgbImage copyRgbImage(RgbImage image);

/**
 * @brief Allocates an empty image in the domain [minX...maxX] x [minY..maxY] with the specified parameters.
 *
 * @param minX The start of the image domain in the x direction.
 * @param maxX The end of the image domain in the x direction.
 * @param minY The start of the image domain in the y direction.
 * @param maxY The end of the image domain in the y direction.
 * @param minValue The minimum possible value this image should be able to contain.
 * @param maxValue The maximum possible value this image should be able to contain.
 * @return RgbImage A newly allocated RgbImage. Note that you should free the resulting image when you are done with it.
 */

RgbImage allocateRgbImageGrid(int minX, int maxX, int minY, int maxY, int minValue, int maxValue);
/**
 * @brief Frees the memory used by the provided image.
 *
 * @param image The image for which to free the memory.
 */
void freeRgbImage(RgbImage image);

/* ----------------------------- Image Getters ----------------------------- */

/**
 * @brief Retrieve the domain information of the provided image.
 *
 * @param image The image from which to retrieve the domain.
 * @return ImageDomain The domain of the image.
 */
ImageDomain getRgbImageDomain(RgbImage image);

/**
 * @brief Retrieve the dynamic range of the provided image.
 *
 * @param image The image from which to retrieve the dynamic range.
 * @param minRange The minimum possible value a channel in this image is able to contain will be put here.
 * @param maxRange The maximum possible value a channel in this image is able to contain will be put here.
 */
void getRgbDynamicRange(RgbImage image, int *minRange, int *maxRange);

/**
 * @brief Retrieves the rgb values of the image at the provided coordinates. Note that x and y can be negative if the
 * image domain allows for this.
 *
 * @param image The image from which to retrieve the rgb value.
 * @param x The x coordinate of the pixel to retrieve.
 * @param y The y coordinate of the pixel to retrieve.
 * @param r The red channel value at (x,y) will be put here.
 * @param g The green channel value at (x,y) will be put here.
 * @param b The blue channel value at (x,y) will be put here.
 */
void getRgbPixel(RgbImage image, int x, int y, int *r, int *g, int *b);

/* ----------------------------- Image Setters ----------------------------- */

/**
 * @brief Set the rgb value of the image at the provided coordinates. Note that x and y can be negative if the
 * image domain allows for this. Additionally, the rgb value should fit in the dynamic range of the image.
 *
 * @param image The image from which to retrieve the rgb value.
 * @param x The x coordinate of the pixel to retrieve.
 * @param y The y coordinate of the pixel to retrieve.
 * @param r The red channel value at (x,y).
 * @param g The green channel value at (x,y).
 * @param b The blue channel value at (x,y).
 */
void setRgbPixel(RgbImage *image, int x, int y, int r, int g, int b);

/**
 * @brief Sets all the pixels in the provided image to the provided rgb value. Note that the rgb values should fit in
 * the dynamic range of the image.
 *
 * @param image The image in which to set all the rgb values.
 * @param r The red channel value to put in the image.
 * @param g The green channel value to put in the image.
 * @param b The blue channel value to put in the image.
 */
void setAllRgbPixels(RgbImage *image, int r, int g, int b);

/* ----------------------------- Image Printing + Viewing ----------------------------- */

/**
 * @brief Prints all the pixel values in the provided image to stdout. Every row is put on a new line.
 * Each rgb value is printed as (r,g,b)
 *
 * @param image The image to print.
 */
void printRgbBuffer(RgbImage image);
/**
 * @brief Prints a LaTeX compatible table representation of the provided image to stdout.
 *
 * @param image The image to print the LaTeX table of.
 */
void printRgbImageLatexTable(RgbImage image);
/**
 * @brief Prints a LaTeX compatible table representation of the provided image to the provided file stream.
 *
 * @param out File stream to print the image to.
 * @param image The image to print the LaTeX table of.
 */
void printRgbLatexTableToFile(FILE *out, RgbImage image);
/**
 * @brief Opens a window that allows the user to view the image. Note that this uses OpenGL which in turn allocates
 * memory that cannot be freed. If you want to check for memory leaks, make sure that you do not run any image displays.
 *
 * @param image The image to view
 * @param windowTitle The title of the window.
 */
void displayRgbImage(RgbImage image, const char *windowTitle);

/* ----------------------------- Image Loading + Saving ----------------------------- */

/**
 * @brief Loads an image from the provided file. Supported extensions: .ppm.
 *
 * @param path The path of the image to load.
 * @return RgbImage An integer image represtation of the provided file.
 */
RgbImage loadRgbImage(const char *path);

/**
 * @brief Saves an image at the provided location. Supported extensions: .ppm. This will save the netpbm
 * file in its binary format.
 *
 * @param image The images to save.
 * @param path The location to save the image at.
 */
void saveRgbImage(RgbImage image, const char *path);

/**
 * @brief Saves an image at the provided location. The location must be a .ppm file. Pixels values are stored as raw
 * bytes in the file.
 *
 * @param image The images to save.
 * @param path The location to save the image at.
 */
void saveRgbImagePPMRaw(RgbImage image, const char *path);

/**
 * @brief Saves an image at the provided location. The location must be a .ppm file. Pixels values are stored as ascii
 * (human readable) values in the file.
 *
 * @param image The images to save.
 * @param path The location to save the image at.
 */
void saveRgbImagePPMAscii(RgbImage image, const char *path);

/* ----------------------------- General Operations ----------------------------- */

/**
 * @brief Creates a new image h from two input images f and g where each pixel is defined as h(x,y) = max(f(x,y),
 * g(x,y)). Performs the operation seperately for each channel.
 *
 * @param imageA First input image
 * @param imageB Second input image
 * @return RgbImage Output image where each pixel is the maximum of the corresponding pixels in the two input images
 */
RgbImage maxRgbImage(RgbImage imageA, RgbImage imageB);

/**
 * @brief Creates a new image h from two input images f and g where each pixel is defined as h(x,y) = min(f(x,y),
 * g(x,y)). Performs the operation seperately for each channel.
 *
 * @param imageA First input image
 * @param imageB Second input image
 * @return IntImage Output image where each pixel is the minimum of the corresponding pixels in the two input images
 */
RgbImage minRgbImage(RgbImage imageA, RgbImage imageB);

/**
 * @brief Creates a new image h from two input images f and g where each pixel is defined as h(x,y) = f(x,y) + g(x,y).
 * Performs the operation seperately for each channel.
 *
 * @param imageA First input image
 * @param imageB Second input image
 * @return IntImage Output image where each pixel is the sum of the corresponding pixels in the two input images
 */
RgbImage addRgbImage(RgbImage imageA, RgbImage imageB);

/**
 * @brief Creates a new image h from two input images f and g where each pixel is defined as h(x,y) = f(x,y) - g(x,y).
 * Performs the operation seperately for each channel.
 *
 * @param imageA First input image
 * @param imageB Second input image
 * @return IntImage Output image where each pixel is the subtraction of the corresponding pixels in the two input images
 */
RgbImage subtractRgbImage(RgbImage imageA, RgbImage imageB);

/**
 * @brief Creates a new image h from two input images f and g where each pixel is defined as h(x,y) = f(x,y) * g(x,y).
 * Performs the operation seperately for each channel.
 *
 * @param imageA First input image
 * @param imageB Second input image
 * @return IntImage Output image where each pixel is the multiplication of the corresponding pixels in the two input
 * images
 */
RgbImage multiplyRgbImage(RgbImage imageA, RgbImage imageB);

/**
 * @brief Produces an output image that is the result of applying a lookup table (LUT) to the input image. The LUT
 * should have the same size as the dynamic range of the input image. Should have a value for each channel.
 *
 * @param image Input image
 * @param LUT The lookup table. Maps a gray value in [minRange..maxRange] of the image to a new gray value.
 * @param LUTSize The size of the lookup table.
 * @return IntImage Image produced from applying the LUT to the input iamge
 */
RgbImage applyLutRgbImage(RgbImage image, int **LUT, int LUTsize);

/* ----------------------------- Image Transformations ----------------------------- */

/**
 * @brief Produces a new, padded image from the provided image. Note that this allocates a new image, which should
 * subsequently be freed.
 *
 * @param image The image to pad.
 * @param top Number of pixels to pad at the top.
 * @param right Number of pixels to pad to the right.
 * @param bottom Number of pixels to pad at the bottom.
 * @param left Number of pixels to pad to the left.
 * @param r Value the red channel of the padded pixels should have.
 * @param g Value the green channel of the padded pixels should have.
 * @param b Value the blue channel of the padded pixels should have.
 * @return RgbImage The newly padded image.
 */
RgbImage padRgbImage(RgbImage image, int top, int right, int bottom, int left, int r, int g, int b);

/**
 * @brief Translates the image domain by x, y. Does not affect the underlying pixel values, only the domain of the
 * image.
 *
 * @param image The image to translate.
 * @param x Number of pixels to translate in the x direction.
 * @param y Number of pixels to translate in the y direction.
 */
void translateRgbImage(RgbImage *image, int x, int y);

/**
 * @brief Horizontally flips an image around the origin.
 *
 * @param image The image to flip.
 */
void flipRgbImageHorizontal(RgbImage *image);

/**
 * @brief Vertically flips an image around the origin.
 *
 * @param image The image to flip.
 */
void flipRgbImageVertical(RgbImage *image);

#endif  // IMPROC_H