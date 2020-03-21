# PPM-ImageFinder
A simple C application that uses forks and pipes to create different processes that search through first-depth sub-directories of a given directory for the closest image to the provided one using euclidean distance

## Installation

* This application is intended to be used on Unix-based machines such as Linux and Mac

Use the provided Makefile for compilation by running the following code in the terminal

```bash
make
```

## Usage

one_process is being used in the provided examples. image_retrieval does the same exact thing using multiple processes ([] are optional)
```terminal
./one_process [-d other] pyramid.ppm
```
