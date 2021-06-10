/**************************************************************
 * Class:  CSC-415-0# 

 * Group: SF STATE

 * Group members: Kevin Ortiz(913050741), Kaung Htun(921325259), Bhupendra Chaudhary(9200113386), Nyan Tun(921138254)
 
 * Project: File System Project
 *
 * File: fsLowDriver.h
 *
 * Description: This is the header file that holds the set of routines used to create a volume
 *              for the Test File System.
 *
 **************************************************************/

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <string.h>

#include <pthread.h>

#include <errno.h>

#include <math.h>

#include <time.h>

#include "fsLow.h"

#include "bitFree.h"

#include "mfs.h"

#define VCB_START_BLOCK 0           // VCB start block assigned to zero
#define DATA_BLOCKS_PER_INODE 4     //Number of data blocks to allocate per on inode.

#ifndef _MAKEVOL
#define _MAKEVOL

//struct that defines what variable are used in Volume Control Block
typedef struct { 
    char header[16];
    uint64_t volumeSize;
    uint64_t blockSize;
    uint64_t diskSizeBlocks;
    uint64_t vcbStartBlock;
    uint64_t totalVCBBlocks;
    uint64_t inodeStartBlock;
    uint64_t totalInodes;
    uint64_t totalInodeBlocks;
    uint64_t freeMapSize;
    uint32_t freeMap[];
}
fs_VCB;
#endif

// will round up integer division
uint64_t ceilDiv(uint64_t, uint64_t);

// Allocates space for an fs_VCB
int allocateVCB(fs_VCB ** );

// if blocks are in the disk, it reads from it
uint64_t fsRead(void * , uint64_t, uint64_t);

// Writes blocks to the disk, edits the freeMap and writes to
uint64_t fsWrite(void * , uint64_t, uint64_t);

// Frees blocks in the freeMap and writes to disk.
void fsFree(void * , uint64_t, uint64_t);

// writes blocks to the disk, edits the freeMap and writes to the disk
int checkIfStorageIsAvalibale(int numberOfRequestedBlocks);

//return the first free block
uint64_t getFreeBlock();

/* Reads the VCB into memory. */
uint64_t readVCB();

/* Writes the VCB to disk. */
uint64_t writeVCB();

/* Gets pointer to the in memory VCB. */
fs_VCB * getVCB();

/* Utility function for printing the VCB in hex and ASCII. */
void printVCB();

// creates the volume
int createVolume(char * , uint64_t, uint64_t);

// opens the volume
void openVolume(char * );

// closes the volume
void closeVolume();
