/**************************************************************
 * Class:  CSC-415-01

 * Group: SF STATE

 * Group members: Kevin Ortiz, Kaung Htun, Bhupendra Chaudhary, Nyan Tun
 
 * Project: File System Project
 *
 * File: fsLowDriver.c
 *
 * Description: This program is used to create a volume for the
 *              Test File System. It holds the free-space bit vector
 *		and the inodes on disk.
 *
 **************************************************************/

#include "fsLowDriver.h"

int initialized = 0; //checks if the VCB is addressed 

// Information about volume control block
char header[16] = "*****TestFS****";
uint64_t volumeSize;
uint64_t blockSize;
uint64_t diskSizeBlocks;
uint32_t vcbStartBlock;
uint32_t totalVCBBlocks;
uint32_t inodeStartBlock;
uint32_t totalInodes;
uint32_t totalInodeBlocks;
uint32_t freeMapSize;

fs_VCB * openVCB_p; //pointer to our volume control block

 /* Rounds up integer division. */
uint64_t ceilDiv(uint64_t a, uint64_t b) {   
    return (a + b - 1) / b;
}
/* It helps in assigning space for an fs_VCB with size set as a multiple of block size and
 * assigns it to the pointer argument and it returns total number of blocks allocated. */
int allocateVCB(fs_VCB ** vcb_p) {
    * vcb_p = calloc(totalVCBBlocks, blockSize);
    return totalVCBBlocks;
}
/* fsRead reads from the disk, if blocks are available. 
 LBAread will read the blockcount and blockposition and will read the block count.
*/
uint64_t fsRead(void * buf, uint64_t blockCount, uint64_t blockPosition) {
    LBAread(buf, blockCount, blockPosition);
    return blockCount;
}
/* fswrite writes from the disk, if blocks are available.It will also check upto how much block will fswrite be able to write.
Also, it will edit the freeMap and writes to the disk.
*/

uint64_t fsWrite(void * buf, uint64_t blockCount, uint64_t blockPosition) {
    LBAwrite(buf, blockCount, blockPosition);
    for (int i = 0; i < blockCount; i++) {
        setBit(openVCB_p -> freeMap, blockPosition + i);
    }
    writeVCB();
    return blockCount;
}
/* when vcb it initialized, fsFree will free the blocks in the freeMap and 
write to the disk.
*/

void fsFree(void * buf, uint64_t blockCount, uint64_t blockPosition) {
    for (int i = 0; i < blockCount; i++) {
        clearBit(openVCB_p -> freeMap, blockPosition + i);
    }
    writeVCB();
}

/* This method receives the next available free block and will return -1 if not.
   And if the free block is found, it's position is addresed by index in return.
*/
uint64_t getFreeBlock() {
    for (int index = 0; index < diskSizeBlocks; index++) {
        if (findBit(openVCB_p -> freeMap, index) == 0) {
            return index; 
        }
    }
    return -1;
}

// read() reads volume control block in the memory
uint64_t readVCB() {
    /* Read VCB from disk to openVCB_p */
    uint64_t blocksRead = LBAread(openVCB_p, totalVCBBlocks, VCB_START_BLOCK);
    return blocksRead;
}

uint64_t writeVCB() {
    /* Write openVCB_p to disk. */
    uint64_t blocksWritten = LBAwrite(openVCB_p, totalVCBBlocks, VCB_START_BLOCK);
    return blocksWritten;
}

fs_VCB * getVCB() {
    return openVCB_p;
}

void initializeVCB() {
    sprintf(openVCB_p -> header, "%s", header);

    /* Set information on volume sizes and block locations. */
    openVCB_p -> volumeSize = volumeSize;
    openVCB_p -> blockSize = blockSize;
    openVCB_p -> diskSizeBlocks = diskSizeBlocks;
    openVCB_p -> vcbStartBlock = VCB_START_BLOCK;
    openVCB_p -> totalVCBBlocks = totalVCBBlocks;
    openVCB_p -> inodeStartBlock = inodeStartBlock;
    openVCB_p -> totalInodes = totalInodes;
    openVCB_p -> totalInodeBlocks = totalInodeBlocks;
    openVCB_p -> freeMapSize = freeMapSize;

    /* Initialize freeBlockMap to all 0's. */
    for (int i = 0; i < freeMapSize; i++) {
        openVCB_p -> freeMap[i] = 0;
    }

    /* Set bits in freeMap for VCB and inodes. */
    for (int i = 0; i < inodeStartBlock + totalInodeBlocks; i++) {
        setBit(openVCB_p -> freeMap, i);
    }

    printVCB();
    writeVCB();
}

void initializeInodes() {

   /* initializing and allocating inodes. First inode is root directory and has id=1.*/
    fdDir * inodes = calloc(totalInodeBlocks, blockSize);
    inodes[0].id = 0;
    inodes[0].inUse = 1;
    inodes[0].type = I_DIR;
    strcpy(inodes[0].name, "root");
    strcpy(inodes[0].path, "/root");
    inodes[0].lastAccessTime = time(0);
    inodes[0].lastModificationTime = time(0);
    inodes[0].numDirectBlockPointers = 0;

    // for other nodes
    for (int i = 1; i < totalInodes; i++) {
        inodes[i].id = i;
        inodes[i].inUse = 0;
        inodes[i].type = I_UNUSED;
        strcpy(inodes[i].parent, "");
        strcpy(inodes[i].name, "");
        inodes[i].lastAccessTime = 0;
        inodes[i].lastModificationTime = 0;

        /* Set all direct block pointers to -1 (invalid). */
        for (int j = 0; j < MAX_DATABLOCK_POINTERS; j++) {
            inodes[i].directBlockPointers[j] = INVALID_DATABLOCK_POINTER;
        }
        inodes[i].numDirectBlockPointers = 0;

    }

    /* Write inodes to disk. */
    char * char_p = (char * ) inodes;
    LBAwrite(char_p, totalInodeBlocks, inodeStartBlock);
    free(inodes);
}
      //printing VCB
void printVCB() {
    int size = openVCB_p -> totalVCBBlocks * (openVCB_p -> blockSize);
    int width = 16;
    char * char_p = (char * ) openVCB_p;
    char ascii[width + 1];
    for (int i = 0; i < size; i++) {
        if (char_p[i]) {
            ascii[i % width] = char_p[i];
        }
        if ((i + 1) % width == 0 && i > 0) {
            ascii[i % width + 1] = '\0';
        } else if (i == size - 1) {
            for (int j = 0; j < width - (i % (width - 1)); j++) {
                printf("   ");
            }
            ascii[i % width + 1] = '\0';
        }
    }
}

void init(uint64_t _volumeSize, uint64_t _blockSize) {
     volumeSize = _volumeSize;
     blockSize = _blockSize;
     diskSizeBlocks = ceilDiv(volumeSize, blockSize);
     freeMapSize = diskSizeBlocks <= sizeof(uint32_t) * 8 ? 1 : diskSizeBlocks / sizeof(uint32_t) / 8;
     totalVCBBlocks = ceilDiv(sizeof(fs_VCB) + sizeof(uint32_t[freeMapSize]), blockSize);
     inodeStartBlock = VCB_START_BLOCK + totalVCBBlocks;
     totalInodes = (diskSizeBlocks - inodeStartBlock) / (DATA_BLOCKS_PER_INODE + ceilDiv(sizeof(fdDir), blockSize));
     totalInodeBlocks = ceilDiv(totalInodes * sizeof(fdDir), blockSize);

    /* Allocate the VCB in memory. */
    int vcbSize = allocateVCB( & openVCB_p);
    initialized = 1;
}

int createVolume(char * volumeName, uint64_t _volumeSize, uint64_t _blockSize) {
    /* Check whether volume exists already. */
    if (access(volumeName, F_OK) != -1) {
        return -3;
    }

    uint64_t existingVolumeSize = _volumeSize;
    uint64_t existingBlockSize = _blockSize;

    /* Initialize the volume with the fsLow library. */
    int returnValue = startPartitionSystem(volumeName, & existingVolumeSize, & existingBlockSize);

    printf(" VolumeName= %s", volumeName);
    printf(" Volume Size= %llu", (ull_t) existingVolumeSize);
    printf(" BlockSize=: %llu",  (ull_t) existingBlockSize);

    /* Format the disk if the volume was opened properly. */
    if (!returnValue) {
        init(_volumeSize, _blockSize);
        initializeVCB();
        initializeInodes();
    }

    closeVolume();
    return returnValue;
}
//opens the volume
void openVolume(char * volumeName) {;
    if (!initialized) {
        uint64_t existingVolumeSize;
        uint64_t existingBlockSize;

        int returnValue = startPartitionSystem(volumeName, & existingVolumeSize, & existingBlockSize);
        if (!returnValue) {
            init(existingVolumeSize, existingBlockSize);
            readVCB();

            printVCB();
        }
    } else {
        printf("volume is already open.\n", volumeName);
    }
}
// closes the volume
void closeVolume() {
    if (initialized) {
        closePartitionSystem();
        free(openVCB_p);
        initialized = 0;
    } else {
        printf("Volume is not closed.\n");
    }
}
