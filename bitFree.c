/**************************************************************
 * Class:  CSC-415-0#

 * Group: SF STATE

 * Group members: Kevin Ortiz, Kaung Htun, Bhupendra Chaudhary, Nyan Tun
 
 * Project: File System Project
 *
 * File: bitFree.c
 *
 * Description: This file defines the methods that change our free-space bit vector.
 *
 **************************************************************/

#include "bitFree.h"

void setBit(int A[], int k) {
    A[k / 32] |= 1 << (k % 32);
}

void clearBit(int A[], int k) {
    A[k / 32] &= ~(1 << (k % 32));
}

int findBit(int A[], int k) {
    return ((A[k / 32] & (1 << (k % 32))) != 0);
}
