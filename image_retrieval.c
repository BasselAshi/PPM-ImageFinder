/*
    Author: Bassel Ashi
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <float.h>
#include "worker.h"

int main(int argc, char **argv)
{

    char ch;
    char path[PATHLENGTH];
    char pathCount[PATHLENGTH];
    char *startdir = ".";
    char *image_file = NULL;

    while ((ch = getopt(argc, argv, "d:")) != -1)
    {
        switch (ch)
        {
        case 'd':
            startdir = optarg;
            break;
        default:
            fprintf(stderr, "Usage: queryone [-d DIRECTORY_NAME] FILE_NAME\n");
            exit(1);
        }
    }

    if (optind != argc - 1)
    {
        fprintf(stderr, "Usage: queryone [-d DIRECTORY_NAME] FILE_NAME\n");
        exit(1);
    }
    else
        image_file = argv[optind];

    // Counting existing sub-directories
    //-----------------------------------------------------------------------------------------
    // Open the directory provided by the user (or current working directory)

    DIR *dirp;
    if ((dirp = opendir(startdir)) == NULL)
    {
        perror("opendir");
        exit(1);
    }

    DIR *dirpCount;
    if ((dirpCount = opendir(startdir)) == NULL)
    {
        perror("opendir");
        exit(1);
    }

    /* For each entry in the directory, eliminate . and .., and check
	* to make sure that the entry is a directory, then call run_worker
	* to process the image files contained in the directory.
	*/

    struct dirent *dpCount;
    int count = 0;

    while ((dpCount = readdir(dirpCount)) != NULL)
    {

        if (strcmp(dpCount->d_name, ".") == 0 ||
            strcmp(dpCount->d_name, "..") == 0 ||
            strcmp(dpCount->d_name, ".svn") == 0)
        {
            continue;
        }
        strncpy(pathCount, startdir, PATHLENGTH);
        strncat(pathCount, "/", PATHLENGTH - strlen(pathCount) - 1);
        strncat(pathCount, dpCount->d_name, PATHLENGTH - strlen(pathCount) - 1);

        struct stat sbufCount;
        if (stat(pathCount, &sbufCount) == -1)
        {
            //This should only fail if we got the path wrong
            // or we don't have permissions on this entry.
            perror("stat");
            exit(1);
        }

        // Only call process_dir if it is a directory
        // Otherwise ignore it.
        if (S_ISDIR(sbufCount.st_mode))
        {
            count++;
        }
    }
    //-----------------------------------------------------------------------------------------

    Image *img = read_image(image_file); // Read given image
    // Invalid image
    if (img == NULL)
    {
        fprintf(stderr, "Invalid image input!\n");
        exit(1);
    }

    struct dirent *dp;
    CompRecord CRec;
    strcpy(CRec.filename, "");
    CRec.distance = FLT_MAX;
    int fd[2];
    int fds[count];
    int i = 0;

    while ((dp = readdir(dirp)) != NULL)
    {

        if (strcmp(dp->d_name, ".") == 0 ||
            strcmp(dp->d_name, "..") == 0 ||
            strcmp(dp->d_name, ".svn") == 0)
        {
            continue;
        }
        strncpy(path, startdir, PATHLENGTH);
        strncat(path, "/", PATHLENGTH - strlen(path) - 1);
        strncat(path, dp->d_name, PATHLENGTH - strlen(path) - 1);

        struct stat sbuf;
        if (stat(path, &sbuf) == -1)
        {
            //This should only fail if we got the path wrong
            // or we don't have permissions on this entry.
            perror("stat");
            exit(1);
        }

        // Only call process_dir if it is a directory
        // Otherwise ignore it.
        if (S_ISDIR(sbuf.st_mode))
        {
            if (pipe(fd) == -1)
            { // Error
                fprintf(stderr, "Error creating a pipe!\n");
                exit(1);
            }
            fds[i] = fd[0];
            i++;

            int r = fork();
            if (r < 0)
            { // Error
                fprintf(stderr, "Error creating a process!\n");
                exit(1);
            }
            else if (r > 0)
            { // Parent
                close(fd[1]);
            }
            else
            { // Child
                printf("Processing all images in directory: %s \n", path);
                close(fd[0]);
                process_dir(path, img, fd[1]);
                free(img->p);
                free(img);
                close(fd[1]);
                return 0;
            }
        }
    }

    CompRecord tempCR;
    for (int j = 0; j < i; j++)
    {
        while (read(fds[j], &tempCR, sizeof(tempCR)) > 0)
        {
            // Compare found CR with current CR
            if (tempCR.distance < CRec.distance)
            {
                CRec = tempCR;
            }
        }
        close(fds[j]);
    }

    free(img->p);
    free(img);

    printf("The most similar image is %s with a distance of %f\n", CRec.filename, CRec.distance);

    return 0;
}
