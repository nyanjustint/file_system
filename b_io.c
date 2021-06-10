/**************************************************************
 * Class:  CSC-415-0#

 * Group: SF STATE

 * Group members: Kevin Ortiz, Kaung Htun, Bhupendra Chaudhary, Nyan Tun
 
 * Project: File System Project
 *
 * File: b_io.c
 *
 * Description: Interface of basic I/O functions
 *
 **************************************************************/

#include <stdio.h>

#include <unistd.h>

#include <stdlib.h>

#include <string.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include "b_io.h"

#include "mfs.h"

#define MAXFCBS 20


uint64_t bufSize;

typedef enum {
    NoWRITE,
    WRITE
}
fileMode;

typedef struct b_fcb {
    int linuxFd;
    char * buf;
    int index;
    int blockIndex;
    int buflen;
    fileMode mode;
    fdDir * inode;
    int eof;

}
b_fcb;

b_fcb fcbArray[MAXFCBS];

int startup = 0;

//Method to initialize our file system
void b_init() {

    
    bufSize = getVCB() -> blockSize;

    
    for (int i = 0; i < MAXFCBS; i++) { //both indicating a free fcbArray
        fcbArray[i].linuxFd = -1;
        fcbArray[i].mode = NoWRITE;
    }

    startup = 1;
}

//Method to get free element/file in the fcbArray
int b_getFCB() {
    for (int i = 0; i < MAXFCBS; i++) {
        if (fcbArray[i].linuxFd == -1) {
            fcbArray[i].linuxFd = -2;
            return i;
        }
    }
    return (-1);
}

//Buffer open with the permission/flags
int b_open(char * filename, int flags) {
    int fd;
    int returnFd;

    printf("b_open\n");

    //initialize the system
    if (startup == 0) b_init();

    
    returnFd = b_getFCB();

    // Check if there was an available fcb returned to us.
    b_fcb * fcb;
    if (returnFd < 0) {
        return -1;
    }

    fcb = & fcbArray[returnFd];

    //open file with the given filename if not exist, create one
    fdDir * inode = getInode(filename);
    if (!inode) {

        printf("b_open: %s does not yet exist.\n", filename);

        //create the given filename file if the flag is to create
        if (flags & O_CREAT) {

            printf("Creating %s\n", filename);
            inode = createInode(I_FILE, filename);
            char parentpath[MAX_FILENAME_SIZE];
            getParentPath(parentpath, filename);
            fdDir * parent = getInode(parentpath);
            setParent(parent, inode);
            writeInodes();

        }
        else {
            return -1;
        }
    }

    fcb -> inode = inode;

    // allocate our buffer
    fcb -> buf = malloc(bufSize + 1);
    if (fcb -> buf == NULL) {
        close(fd);
        fcb -> linuxFd = -1;
        return -1;
    }

    fcb -> buflen = 0;
    fcb -> index = 0;

    return (returnFd);
}

// buffer write
int b_write(int fd, char * buffer, int count) {
    //initialize if not
    if (startup == 0) {
        b_init();
    }

    if ((fd < 0) || (fd >= MAXFCBS)) {
        return (-1);
    }

    if (fcbArray[fd].linuxFd == -1)
    {
        return -1;
    }

    // Calculate free space in buffer of FCB

    b_fcb * fcb = & fcbArray[fd];
    int freeSpace = bufSize - fcb -> index;

    // set the file to write the last bytes in b_close
    fcb -> mode = WRITE;

    int part1 = freeSpace > count ? count : freeSpace;
    int part2 = count - part1;

    memcpy(fcb -> buf + fcb -> index, buffer, part1);
    fcb -> index += part1;
    fcb -> index %= bufSize;

   
    if (part2 != 0) {
        uint64_t indexOfBlock = getFreeBlock();


        if (indexOfBlock == -1) {
            printf("There is not enough free space!");
            return 0;
        } else {

            
            writeBufferToInode(fcb -> inode, fcb -> buf, part1 + part2, indexOfBlock);
        }
        fcb -> index = 0;
        
        memcpy(fcb -> buf + fcb -> index, buffer + part1, part2);
        fcb -> index += part2;
        fcb -> index %= bufSize;
    }

    return part1 + part2;
}

//buffer read
int b_read(int fd, char * buffer, int count) {
    struct b_fcb * fcb = & fcbArray[fd];
    
    int bytesRemaining = fcb -> buflen - fcb -> index;

    if (bytesRemaining > count) {

        //Copy from existing buffer
        memcpy(buffer, fcb -> buf + fcb -> index, count);
        fcb -> index += count;
        return count;
    }
    else {
        //Copy tail-end of existing buffer
        memcpy(buffer, fcb -> buf + fcb -> index, bytesRemaining);

        if (fcb -> eof) {
            
            fcb -> index += bytesRemaining;
            return bytesRemaining;
        }

        //Check if inode has a direct block pointer to read from
        if (fcb -> blockIndex > fcb -> inode -> numDirectBlockPointers - 1) {
            printf("Block Index out-of-bounds.\n");
            return 0;
        }

        //Read the next data block pointer and reset the index
        int blockNumber = fcb -> inode -> directBlockPointers[fcb -> blockIndex];
        LBAread(fcb -> buf, 1, blockNumber);
        fcb -> blockIndex++;

       
        fcb -> index = 0;
        int newBufferSize = fcb -> buflen = strlen(fcb -> buf);
       

        
        if (newBufferSize < bufSize) {
            fcb -> eof = 1;
        }
        int remainderOfCount = count - bytesRemaining;
        int secondSegmentCount = newBufferSize > remainderOfCount ? remainderOfCount : newBufferSize;

        memcpy(buffer + bytesRemaining, fcb -> buf + fcb -> index, secondSegmentCount);

        fcb -> index += secondSegmentCount;
        return bytesRemaining + secondSegmentCount;
    }
}

// buffer close
void b_close(int fd) {
    b_fcb * fcb = & fcbArray[fd];
   

    if (fcb -> mode == WRITE && fcb -> index > 0) {
       
        uint64_t indexOfBlock = getFreeBlock();
        if (indexOfBlock == -1) {
            printf("There is not enough free space!");
            return;
        }
        else {
           
            writeBufferToInode(fcb -> inode, fcb -> buf, fcb -> index, indexOfBlock);
        }

    }

    close(fcb -> linuxFd);
    free(fcb -> buf);
    fcb -> buf = NULL;
    fcb -> linuxFd = -1;
}
