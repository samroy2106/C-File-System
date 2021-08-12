#include <stdio.h>
#include <stdlib.h>
#include "diskdriver.h"

const int BLOCK_SIZE = 512;
const int NUM_BLOCKS = 4096;

//Reads one block at a time from disk
void readBlock(FILE* disk, int blockNum, char* buffer){
  fseek(disk, blockNum * BLOCK_SIZE, SEEK_SET);
  fread(buffer, BLOCK_SIZE, 1, disk);
}

//Writes one block at a time to disk
void writeBlock(FILE* disk, int blockNum, char* data){
  fseek(disk, blockNum * BLOCK_SIZE, SEEK_SET);
  fwrite(data, BLOCK_SIZE, 1, disk);
}
