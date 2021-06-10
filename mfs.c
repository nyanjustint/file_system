/**************************************************************
 * Class:  CSC-415-01
	
 * Group: SF STATE

 * Group members: Kevin Ortiz, Kaung Htun, Bhupendra Chaudhary, Nyan Tun
 
 * Project: Basic File System
 *
 * File: mfs.c
 *
 * Description: This file defines the methods to initialize, keep track of, and change our file system's inodes.
 *
 **************************************************************/

#include "mfs.h"

fdDir * inodes;

char inodeTypeNames[3][64] = {
    "I_FILE",
    "I_DIR",
    "I_UNUSED"
};

char currentDirectoryPath[MAX_FILEPATH_SIZE];
char currentDirectoryPathArray[MAX_DIRECTORY_DEPTH][MAX_FILENAME_SIZE];
int currentDirectoryPathArraySize = 0;

char requestedFilePath[MAX_FILEPATH_SIZE];
char requestedFilePathArray[MAX_DIRECTORY_DEPTH][MAX_FILENAME_SIZE];
int requestedFilePathArraySize = 0;

size_t NumberOfElementsInInodesArray = sizeof(inodes) / sizeof(inodes[0]); // calculate the number of elemnts in inodes array

void fs_init() {

    uint64_t totalBytes = getVCB() -> totalInodeBlocks * getVCB() -> blockSize;
    printf("totalInodeBlocks %ld, blockSize %ld\n", getVCB() -> totalInodeBlocks, getVCB() -> blockSize);

    inodes = calloc(getVCB() -> totalInodeBlocks, getVCB() -> blockSize);
    printf("Inodes allocated at %p.\n", inodes);

    uint64_t blocksRead = LBAread(inodes, getVCB() -> totalInodeBlocks, getVCB() -> inodeStartBlock);
    printf("Loaded %ld blocks of inodes into cache.\n", blocksRead);

    fs_setcwd("/root");//set to primary directory
}

void writeInodes() {
    LBAwrite(inodes, getVCB() -> totalInodeBlocks, getVCB() -> inodeStartBlock);
}

char * getInodeTypeName(char * buf, InodeType type) {
    strcpy(buf, inodeTypeNames[type]);
    return buf;
}

void parseFilePath(const char * pathname) {

    requestedFilePath[0] = '\0';
    requestedFilePathArraySize = 0;

    char _pathname[MAX_FILEPATH_SIZE];
    strcpy(_pathname, pathname);

    char * savePointer;
    char * token = strtok_r(_pathname, "/", & savePointer);

    int isAbsolute = pathname[0] == '/';
    int isSelfRelative = !strcmp(token, ".");
    int isParentRelative = !strcmp(token, "..");

    if (token && !isAbsolute) {
        int maxLevel = isParentRelative ? currentDirectoryPathArraySize - 1 : currentDirectoryPathArraySize;
        for (int i = 0; i < maxLevel; i++) {
            strcpy(requestedFilePathArray[i], currentDirectoryPathArray[i]);
            sprintf(requestedFilePath, "%s/%s", requestedFilePath, currentDirectoryPathArray[i]);
            requestedFilePathArraySize++;
        }
    }
    if (isSelfRelative || isParentRelative) {
        token = strtok_r(0, "/", & savePointer);
    }

    while (token && requestedFilePathArraySize < MAX_DIRECTORY_DEPTH) {

        strcpy(requestedFilePathArray[requestedFilePathArraySize], token);
        sprintf(requestedFilePath, "%s/%s", requestedFilePath, token);

        requestedFilePathArraySize++;
        token = strtok_r(0, "/", & savePointer);

    }

    printf("Requested file name '%s'\n", requestedFilePath);

}


fdDir * getInode(const char * pathname) {
    for (size_t i = 0; i < 6; i++) {
        if (strcmp(inodes[i].path, pathname) == 0) {
            return &inodes[i];
        }
    }

    printf("Path '%s' does not exist.\n", pathname);

    return NULL;

}

fdDir * getFreeInode() {
    fdDir * returnediNode;

    for (size_t i = 0; i < getVCB() -> totalInodes; i++) {
        if (inodes[i].inUse == 0) { 
            inodes[i].inUse = 1; 
            returnediNode = & inodes[i];

            return returnediNode;

        }
    }
    return NULL;
}

fdDir * createInode(InodeType type,
    const char * path) {
    fdDir * inode;
    char parentPath[MAX_FILEPATH_SIZE];
    fdDir * parentNode;

    time_t currentTime;
    currentTime = time(NULL);

    inode = getFreeInode();

    getParentPath(parentPath, path);
    parentNode = getInode(parentPath);

    inode -> type = type;
    strcpy(inode -> name, requestedFilePathArray[requestedFilePathArraySize - 1]);
    sprintf(inode -> path, "%s/%s", parentPath, inode -> name);
    inode -> lastAccessTime = currentTime;
    inode -> lastModificationTime = currentTime;

    if (!setParent(parentNode, inode)) {
        freeInode(inode);
        return NULL;
    }
    return inode;

}

int parentHasChild(fdDir * parent, fdDir * child) {
    for (int i = 0; i < parent -> numChildren; i++) {
        if (!strcmp(parent -> children[i], child -> name)) {
            return 1;
        }
    }

    return 0;
}

int setParent(fdDir * parent, fdDir * child) {

    if (parent -> numChildren == MAX_NUMBER_OF_CHILDREN) {
        return 0;
    }

    if (parentHasChild(parent, child)) {
        return 0;
    }
    strcpy(parent -> children[parent -> numChildren], child -> name);
    parent -> numChildren++;
    parent -> lastAccessTime = time(0);
    parent -> lastModificationTime = time(0);
    parent -> sizeInBlocks += child -> sizeInBlocks;
    parent -> sizeInBytes += child -> sizeInBytes;

    strcpy(child -> parent, parent -> path);
    sprintf(child -> path, "%s/%s", parent -> path, child -> name);

    return 1;
}

int removeFromParent(fdDir * parent, fdDir * child) {
   
    for (int i = 0; i < parent -> numChildren; i++) {

        if (!strcmp(parent -> children[i], child -> name)) {
            strcpy(parent -> children[i], "");
            parent -> numChildren--;
            parent -> sizeInBlocks -= child -> sizeInBlocks;
            parent -> sizeInBytes -= child -> sizeInBytes;
            return 1;
        }
    }
    return 0;

}
char * getParentPath(char * buf,
    const char * path) {

    parseFilePath(path);

    char parentPath[MAX_FILEPATH_SIZE] = "";
    for (int i = 0; i < requestedFilePathArraySize - 1; i++) {
        strcat(parentPath, "/");
        strcat(parentPath, requestedFilePathArray[i]);
    }

    strcpy(buf, parentPath);

    return buf;
}

fdDir * getInodeByID(int id) {
    if (0 <= id < getVCB() -> totalInodes) {
        return &inodes[id];
    } else {
        return NULL;
    }
}

int writeBufferToInode(fdDir * inode, char * buffer, size_t bufSizeBytes, uint64_t blockNumber) {

    int freeIndex = -1;
    for (int i = 0; i < MAX_DATABLOCK_POINTERS; i++) {
        if (inode -> directBlockPointers[i] == INVALID_DATABLOCK_POINTER) {

            freeIndex = i;
            break;
        }
    }
    if (freeIndex == -1) {
        return 0;
    }
    LBAwrite(buffer, 1, blockNumber);
    inode -> directBlockPointers[freeIndex] = blockNumber;
    setBit(getVCB() -> freeMap, blockNumber);
    writeVCB();

    inode -> numDirectBlockPointers++;
    inode -> sizeInBlocks++;
    inode -> sizeInBytes += bufSizeBytes;
    inode -> lastAccessTime = time(0);
    inode -> lastModificationTime = time(0);

    writeInodes();

    return 1;

}

void freeInode(fdDir * node) {

    node -> inUse = 0;
    node -> type = I_UNUSED;
    node -> name[0] = NULL;
    node -> path[0] = NULL;
    node -> parent[0] = NULL;
    node -> sizeInBlocks = 0;
    node -> sizeInBytes = 0;
    node -> lastAccessTime = 0;
    node -> lastModificationTime = 0;
    if (node -> type == I_FILE) {
        for (size_t i = 0; i < node -> numDirectBlockPointers; i++) {
            int blockPointer = node -> directBlockPointers[i];
            clearBit(getVCB() -> freeMap, blockPointer);
        }
    }

    writeInodes();

}

void fs_close() {

    free(inodes);

}

int fs_mkdir(const char * pathname, mode_t mode) {

    char parentPath[256] = "";
    parseFilePath(pathname);

    for (size_t i = 0; i < requestedFilePathArraySize - 1; i++) {
        strcat(parentPath, "/");
        strcat(parentPath, requestedFilePathArray[i]);
    }

    fdDir * parent = getInode(parentPath);
    if (parent) {
        for (size_t i = 0; i < parent -> numChildren; i++) {
            if (strcmp(parent -> children[i], requestedFilePathArray[requestedFilePathArraySize - 1])) {
                printf("Folder already existed!\n");
                return -1;
            }
        }
    } else {
        return -1;
    }

    if (createInode(I_DIR, pathname)) {
        writeInodes();
        return 0;
    }
    printf("Error! fail to make '%s'.\n", pathname);
    return -1;
}
int fs_rmdir(const char * pathname) {
    fdDir * node = getInode(pathname);
    if (!node) {
        return -1;
    }
    fdDir * parent = getInode(node -> parent);
    if (node -> type == I_DIR && node -> numChildren == 0) {
        removeFromParent(parent, node);
        freeInode(node);
        printf("Remove '%s'\n",pathname);
        return 0;
    }
    return -1;
}

fdDir * fs_opendir(const char * fileName) {
    int ret = b_open(fileName, 0);
    if (ret < 0) {
        return NULL;
    }
    return getInode(fileName);
}

int readdirCounter = 0;
struct fs_diriteminfo directoryEntry;

struct fs_diriteminfo * fs_readdir(fdDir * dirp) {

    if (readdirCounter == dirp -> numChildren) {
        readdirCounter = 0;
        return NULL;
    }

    char childPath[MAX_FILEPATH_SIZE];
    sprintf(childPath, "%s/%s", dirp -> path, dirp -> children[readdirCounter]);
    fdDir * child = getInode(childPath);

    directoryEntry.d_ino = child -> id;
    strcpy(directoryEntry.d_name, child -> name);

    readdirCounter++;
    return &directoryEntry;
}

int fs_closedir(fdDir * dirp) {
    // 
    return 0;
}

char * fs_getcwd(char * buf, size_t size) {
    if (strlen(currentDirectoryPath) > size) {
        errno = ERANGE;
        return NULL;
    }
    strcpy(buf, currentDirectoryPath);
    return buf;
}

int fs_setcwd(char * buf) {

    parseFilePath(buf);

    fdDir * inode = getInode(requestedFilePath);
    if (!inode) {
        printf("Path '%s' does not exist.\n", requestedFilePath);
        return 1;
    }

    currentDirectoryPath[0] = '\0';
    currentDirectoryPathArraySize = 0;

    for (int i = 0; i < requestedFilePathArraySize; i++) {
        strcpy(currentDirectoryPathArray[i], requestedFilePathArray[i]);
        sprintf(currentDirectoryPath, "%s/%s", currentDirectoryPath, requestedFilePathArray[i]);
        currentDirectoryPathArraySize++;
    }

    printf("Current path is '%s'.\n", currentDirectoryPath);
    return 0;

}

int fs_isFile(char * path) {
    fdDir * inode = getInode(path);
    return inode ? inode -> type == I_FILE : 0;
}

int fs_isDir(char * path) {
    fdDir * inode = getInode(path);
    return inode ? inode -> type == I_DIR : 0;
}

int fs_delete(char * filePath) {
    fdDir * fileNode = getInode(filePath);
    fdDir * parentNode = getInode(fileNode -> parent);
    removeFromParent(parentNode, fileNode);
    freeInode(fileNode);
    return 0;
}

int fs_stat(const char * path, struct fs_stat * buf) {
    fdDir * inode = getInode(path);
    if (inode) {
        buf -> st_size = 999;
        buf -> st_blksize = getVCB() -> blockSize;
        buf -> st_blocks = 2;
        buf -> st_accesstime = 1;
        buf -> st_modtime = 1;
        buf -> st_createtime = 1;
        return 1;
    }
    return 0;
}
