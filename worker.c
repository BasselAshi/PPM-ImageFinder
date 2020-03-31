/*
	Author: Bassel Ashi
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <math.h>
#include <float.h>
#include "worker.h"

/*
 * Read an image from a file and create a corresponding 
 * image struct 
 */

Image *read_image(char *filename)
{
	FILE *filePointer;
	Image *imgPointer = (Image *)malloc(sizeof(Image));
	char fileHeader[5];

	// Check opening file
	if ((filePointer = fopen(filename, "r")) == NULL)
	{
		fprintf(stderr, "Error opening file!\n");
		exit(1);
	}

	// Check file header
	fscanf(filePointer, "%4s", fileHeader);
	if (strcmp(fileHeader, "P3") != 0)
	{
		return NULL;
	}

	// Populate width and height
	int *width = &(imgPointer->width);
	int *height = &(imgPointer->height);
	fscanf(filePointer, "%d %d", width, height);

	// Populate max value
	int *maxValue = &(imgPointer->max_value);
	fscanf(filePointer, "%d", maxValue);

	// Populating pixels
	int dim = (*width) * (*height);
	int i = 0;
	Pixel *pxlsPointer = (Pixel *)malloc(sizeof(Pixel) * dim);
	while (i < dim)
	{
		fscanf(filePointer, "%d %d %d", &(pxlsPointer[i].red), &(pxlsPointer[i].green), &(pxlsPointer[i].blue));
		i++;
	}
	imgPointer->p = pxlsPointer;

	// Check closing file
	if (fclose(filePointer) != 0)
	{
		fprintf(stderr, "Error closing file!\n");
		exit(1);
	}

	return imgPointer;
}

/*
 * Print an image based on the provided Image struct
 */

void print_image(Image *img)
{
	printf("P3\n");
	printf("%d %d\n", img->width, img->height);
	printf("%d\n", img->max_value);

	for (int i = 0; i < img->width * img->height; i++)
		printf("%d %d %d  ", img->p[i].red, img->p[i].green, img->p[i].blue);
	printf("\n");
}

/*
 * Compute the Euclidian distance between two pixels
 */
float eucl_distance(Pixel p1, Pixel p2)
{
	return sqrt(pow(p1.red - p2.red, 2) + pow(p1.blue - p2.blue, 2) + pow(p1.green - p2.green, 2));
}

/*
 * Compute the average Euclidian distance between the pixels
 * in the image provided by img1 and the image contained in
 * the file filename
 */

float compare_images(Image *img1, char *filename)
{
	Image *img2 = read_image(filename);

	// In case img2 is not an image
	if (img2 == NULL)
	{
		return FLT_MAX;
	}

	int widthTwo = img2->width;
	int heightTwo = img2->height;
	int widthOne = img1->width;
	int heightOne = img1->height;

	// Return max if dimensions are not the same
	if (widthOne != widthTwo || heightOne != heightTwo)
	{
		free(img2->p);
		free(img2);
		return FLT_MAX;
	}

	// Calculate the average euclidean distance
	int dim = widthOne * heightOne;
	float sumEuc = 0;
	int i = 0;
	while (i < dim)
	{
		sumEuc += eucl_distance(img1->p[i], img2->p[i]);
		i++;
	}
	float avgEuc = sumEuc / dim;

	free(img2->p);
	free(img2);
	return avgEuc;
}

/* process all files in one directory and find most similar image among them
* - open the directory and find all files in it
* - for each file read the image in it
* - compare the image read to the image passed as parameter
* - keep track of the image that is most similar
* - write a struct CompRecord with the info for the most similar image to out_fd
*/
CompRecord process_dir(char *dirname, Image *img, int out_fd)
{
	// Open directory
	DIR *mainDir;
	if ((mainDir = opendir(dirname)) == NULL)
	{
		perror("opendir");
		exit(1);
	}

	// Initialize CompRecord
	CompRecord CRec;
	strcpy(CRec.filename, "");
	CRec.distance = FLT_MAX;

	// Read directory
	struct dirent *currFile;
	while ((currFile = readdir(mainDir)) != NULL)
	{
		// Copy filename to a variable
		char *fileName = currFile->d_name;

		// If . or ..
		if (strcmp(fileName, "..") == 0 || strcmp(fileName, ".") == 0 || strcmp(fileName, ".svn") == 0)
		{
			continue;
		}

		// Create full file name with path
		char filePath[PATHLENGTH];
		strncpy(filePath, dirname, PATHLENGTH);
		strncat(filePath, "/", PATHLENGTH - strlen(filePath) - 1);
		strncat(filePath, fileName, PATHLENGTH - strlen(filePath) - 1);

		// Get file stats
		struct stat currStat;
		if (stat(filePath, &currStat) == -1)
		{
			perror("stat");
			exit(1);
		}

		// If file
		if (S_ISREG(currStat.st_mode))
		{
			float currentValue = compare_images(img, filePath);
			// Compare is less
			if (currentValue < CRec.distance)
			{
				CRec.distance = currentValue;
				strcpy(CRec.filename, fileName);
			}
		}
	}

	// Redirect output
	if (out_fd != -1)
	{
		if (write(out_fd, &CRec, sizeof(CRec)) == -1)
		{
			perror("write to pipe");
			exit(1);
		}
	}

	return CRec;
}