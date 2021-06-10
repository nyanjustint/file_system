/**************************************************************
* Class:  CSC-415
* Name: Professor Bierman

* Group: SF STATE

* Group members: Kevin Ortiz, Kaung Htun, Bhupendra Chaudhary, Nyan Tun

* Project: Basic File System
*
* File: mfs.h
*
* Description: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/
#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "fsLowDriver.h"
#include "b_io.h"

#define MAX_FILEPATH_SIZE 225
#define	MAX_FILENAME_SIZE 20
#define MAX_DIRECTORY_DEPTH 10
#define MAX_NUMBER_OF_CHILDREN 64		
#define MAX_DATABLOCK_POINTERS	64
#define INVALID_DATABLOCK_POINTER -1
#define INVALID_INODE_NAME	"unused_inode"

#include <dirent.h>
#define FT_REGFILE    DT_REG
#define FT_DIRECTORY DT_DIR
#define FT_LINK    DT_LNK

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

struct fs_diriteminfo
{
    unsigned short d_reclen;    /* length of this record */
    unsigned char fileType;
    char d_name[256];             /* filename max filename is 255 characters */
           
    ino_t d_ino;
    off_t d_off;
};

/* This is the equivalent to an inode in the Unix file system. */

typedef enum { I_FILE, I_DIR, I_UNUSED } InodeType;

char* getInodeTypeName(char* buf, InodeType type);

typedef struct
{
		/*****TO DO:  Fill in this structure with what your open/read directory needs  *****/
        unsigned short  d_reclen;        /*length of this record */
        unsigned short    dirEntryPosition;    /*which directory entry position, like file pos */
        uint64_t    directoryStartLocation;        /*Starting LBA of directory */

        uint64_t id; // holds the index of the inode
        int inUse; //boolean to check if it's being used
        InodeType type; // holds the type of the inode
        char parent[MAX_FILEPATH_SIZE];  // holds the parent path
        char children[MAX_NUMBER_OF_CHILDREN][MAX_FILENAME_SIZE]; // array holding children name and size
        int numChildren;
        char name[MAX_FILENAME_SIZE];
        char path[MAX_FILEPATH_SIZE];
        time_t lastAccessTime;
        time_t lastModificationTime; //  holds time last modife
        blkcnt_t sizeInBlocks;
        off_t sizeInBytes;
        int directBlockPointers[MAX_DATABLOCK_POINTERS];
        int numDirectBlockPointers;


} fdDir;

// Changed fs_setcwd to return an int instead of char*
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);
fdDir * fs_opendir(const char *fileName);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

char * fs_getcwd(char *buf, size_t size);
int fs_setcwd(char *buf);   //linux chdir

int fs_isFile(char * path);    //return 1 if file, 0 otherwise
int fs_isDir(char * path);        //return 1 if directory, 0 otherwise
int fs_delete(char* filename);    //removes a file

// additional functions

void fs_init();
void writeInodes();
void fs_close();

void parseFilePath(const char *pathname);

fdDir* getInode(const char *pathname);
fdDir* getFreeInode();

fdDir* createInode(InodeType type,const char* path);
int setParent(fdDir* parent, fdDir* child);
char* getParentPath(char* buf ,const char* path);

fdDir* getInodeByID(int id);

/* Writes a buffer to a provided data block, adds blockNumber to inode, updates size and timestamps
 * of inode, writes inodes to disk. Returns number of blocks written. */
int writeBufferToInode(fdDir* inode, char* buffer, size_t bufSizeBytes, uint64_t blockNumber);

void freeInode(fdDir* node);



struct fs_stat {
	
	off_t     st_size;    /* total size, in bytes */
	blksize_t st_blksize; /* blocksize for file system I/O */
	blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
	time_t    st_accesstime;   /* time of last access */
	time_t    st_modtime;   /* time of last modification */
	time_t    st_createtime;   /* time of last status change */
	
	/* add additional attributes here for your file system */
    	dev_t     st_dev; //ID
    	ino_t     st_ino; 
    	mode_t    st_mode;
    	nlink_t   st_nlink;
    	uid_t     st_uid;
    	gid_t     st_gid;
    	dev_t     st_rdev;
};

int fs_stat(const char *path, struct fs_stat *buf);

#endif

