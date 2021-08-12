#define vdisk_path "../disk/vdisk"
#define BLOCK_SIZE 512
#define NUM_BLOCKS 4096
#define NUM_INODES 128 //spanning 8 blocks (block 10 to 17)
#define INODE_SIZE 32

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "File.h"
#include "../disk/diskdriver.h"

//Reads one inode at a time from the inode area of the disk
void readInode(FILE* disk, int inodeIndex, char* buffer){
  fseek(disk, 10 * BLOCK_SIZE + inodeIndex * INODE_SIZE, SEEK_SET);
  fread(buffer, INODE_SIZE, 1, disk);
}

//Writes one inode at a time to the inode area of the disk
void writeInode(FILE* disk, int inodeIndex, char* data){
  fseek(disk, 10 * BLOCK_SIZE + inodeIndex * INODE_SIZE, SEEK_SET);
  fwrite(data, INODE_SIZE, 1, disk);
}

void initSuperBlock(FILE* disk){

  char* superblock;
  superblock = (char*)malloc(BLOCK_SIZE);

  int magic = 42;
  int blocks = NUM_BLOCKS;
  int inodes = NUM_INODES;

  //Doing this since first 4 bytes are magic number and size of magic number is only 2 bytes
  memcpy(superblock, &magic, sizeof(int));
  //Similar idea as above because num_blocks is also 2 bytes
  memcpy(superblock + sizeof(int), &blocks, sizeof(int));
  //Similarly, next 4 bytes are num_indoes
  memcpy(superblock + 2 * sizeof(int), &inodes, sizeof(int));

  writeBlock(disk, 0, superblock);
  free(superblock);
}

void initFreeBlockVector(FILE* disk){

    char* fbv = (char*)malloc(BLOCK_SIZE);

    //Set first 18 bits as 1 to signify they are unavailable
    //First 2 bytes are 1, 5th byte is c0
    char c = 0xff;
    for(int i = 0; i < 2; i++){
      memcpy(fbv + i, &c, sizeof(char));
    }

    c = 0xc0;
    memcpy(fbv + 2, &c, sizeof(char));

    c = 0;
    for(int j = 3; j < 512; j++){
      memcpy(fbv + j, &c, sizeof(char));
    }

    writeBlock(disk, 1, fbv);
    free(fbv);
}

//Located in block 2. 406 bytes long, each entry 1 byte long (0 for free, 1 for taken). Terminated using \0
void initFreeInodeList(FILE* disk){

  char* fil = (char*)malloc(BLOCK_SIZE);

  //Initialize all 128 bytes to 0 as all are available to use
  char c = 0;
  for(int i = 0; i < 128; i++){
    memcpy(fil + i, &c, sizeof(char));
  }

  writeBlock(disk, 2, fil);
  free(fil);
}

int findFreeInode(FILE* disk){

  char fil[BLOCK_SIZE] = "";
  int index;

  //read in the freeInodeList
  readBlock(disk, 2, fil);
  //return the index of the first inode that isn't allocated (i.e. is marked 0 in the list)
  for(int i = 0; i < 128; i++){
    if(fil[i] == 0x00){
      index = i;
      return index;
    }
  }

  printf("Max inode limit (128) reached. No more memory available to allocate for inodes.\n");
  exit(0);
}

//if required number of freeblocks are found, exit the loop and return the indices
//else the block is over and freeblocks not found, print not enough space error message
int* findFreeBlocks(FILE* disk, int numBlocks, int* freeBlocks){

  char fbv[BLOCK_SIZE] = "";
  char byte;
  int bitIndex = 0;
  int blockCounter = 0;

  readBlock(disk, 1, fbv);

  for(int i = 3; i < BLOCK_SIZE; i++){

    if(blockCounter == numBlocks){
      return freeBlocks;
    }

    byte = fbv[i];

    //examine the 8 bits, if any are 0, add that index to the freeblocks int array, increment the block counter
    bitIndex = 0;
    while(bitIndex < 8){
      if(byte & 0x01){
        continue; //continue because bit is 1, block is occupied
      }
      else{
        freeBlocks[blockCounter] = bitIndex + (i-1)*8;
        blockCounter++;
      }

      bitIndex++;
      byte = byte >> 1; //rightshift the byte by one and always compare the rightmost bit everytime
    }
  }

  printf("Not enough free blocks on disk to store the file.\n");
  exit(0);
}

void createInode(int index, int type, int size, FILE* disk){

  char* inode = (char*)malloc(INODE_SIZE);

  memcpy(inode, &size, sizeof(int));  //set the size of the directory file
  memcpy(inode + sizeof(int), &type, sizeof(int)); //Set the filetype to directory (0)

  //Initialize the rest of the 24 bytes (i.e. block pointers) to all 0s
  char c = 0;

  for(int i = 8; i < 32; i++){
    memcpy(inode + i, &c, sizeof(char));
  }

  writeInode(disk, index, inode);
  free(inode);
}

//Set the status of the inode at the index passed
void updateFreeInodeList(FILE* disk, int index, char status){

  char* fil = (char*)malloc(BLOCK_SIZE);

  //read in the freeInodeList
  readBlock(disk, 2, fil);

  //update index with status
  fil[index] = status;

  //write the block back
  writeBlock(disk, 2, fil);
  free(fil);
}

void updateFreeBlockVector(FILE* disk, int index, int status){

  char* fbv = (char*)malloc(BLOCK_SIZE);
  unsigned char byte;

  int byteNum = index/8;  //index to byte on vector
  int bitNum = index%8; //bit number in the byte to be updated

  readBlock(disk, 1, fbv);

  byte = fbv[byteNum];  //Pull in the byte containaing the bit to be updated

  //update index with status
  if(status == 0){
    byte &= ~(1 << bitNum);
  }else if(status == 1){
    byte |= 1 << bitNum;
  }

  fbv[byteNum] = byte;

  writeBlock(disk, 1, fbv);
  free(fbv);
}

void initRootDirectory(FILE* disk){

  initFreeInodeList(disk);

  int index = findFreeInode(disk); //1st inode since we just initialized
  createInode(index, 0, 0, disk); //index, type, size, destination
  updateFreeInodeList(disk, index, 1); //Set the index at which the inode was create to unavailable (1)
}

int openDirectory(char* path, FILE* disk){

  //deal with depth of 4 here
  //convert the char to int to be returned on finding it
  int numTokens = 0;
  char* token = path;
  int inodeIndex = 0; //root directory inode is loaded in first iteration

  //Assign the string left after '/' delimiter to token, count number of tokens, repeat till the path is exhausted
  while((token = strchr(token, '/')) != NULL){
    numTokens++;
    token++;
  }

  //if length is more than 3, reject the dir/file creation and print error message saying depth of more than 4 is not permitted
  if(numTokens < 4){

    char* pathArray[numTokens]; //init a string array to store the tokenized path
    const char delim[2] = "/";
    char* pathWithoutRoot = (char*)malloc(100); //arbitrary size
    int i = 1;

    memcpy(pathWithoutRoot, path + 1, 100); //copy the contents of the path minus the leading '/'

    pathArray[0] = strtok(pathWithoutRoot, delim);

    while(path != NULL){

      pathArray[i] = pathWithoutRoot; //copy the token over to the path token array
      //printf("Path array contents: %s\n", pathArray[i]);

      pathWithoutRoot = strtok(NULL, delim);  //get the next token
      i++;
    }

    free(pathWithoutRoot);

    char* dirInode = (char*)malloc(INODE_SIZE);

    char* dirBlock = (char*)malloc(BLOCK_SIZE);
    char* entry = (char*)malloc(INODE_SIZE);
    char* entryStr = (char*)malloc(31);
    short int blockNum;
    int entryFound; //0 is not found, 1 is found

    for(int i = 0; i < numTokens; i++){
      entryFound = 0;
      readInode(disk, inodeIndex, dirInode);

      //fetch the char inode index
      for(int j = 0; j < 10; j++){

        if(entryFound == 1){
          break;
        }

        memcpy(&blockNum, dirInode + 8 + j*2, 2);
        readBlock(disk, blockNum, dirBlock);

        for(int k = 0; k < 15; k++){

          memcpy(entry, dirBlock + k*INODE_SIZE, INODE_SIZE); //Copy the entry over from the block to a string
          memcpy(entryStr, entry + 1, 31);  //Copy the entry string to a separate string entryStr that does not have the 1st size byte for comparison

          if(strcmp(pathArray[i], entryStr) == 0){

            memcpy(&inodeIndex, entry, sizeof(int)); //copy the char inode index to our int inodeIndex, it determines the next inode to visit

            //if index is 0, print error message saying dir not found
            //else if the index is not 0, convert to int and set inodeIndex to it so that it is the next node read
            if(inodeIndex == 0){
              printf("A directory in the path specfied does not exist. Try again by entering a valid path.\n");
              exit(0);
            }
            else{
              entryFound = 1;
              break;
            }
          }
        }
      }
    }

    free(dirBlock);
    free(entry);
    free(entryStr);

    return inodeIndex;  //once loop is exited, return the inodeIndex present in memory as this is the index of the required dir/ file
  }
  else{
    printf("Directory depth of more than 4 (with root directory as depth 1) is not permitted.\n");
    exit(0);
  }
}

//Make a directory entry (filename or directory name) at the first available spot in the first available block pointed to by the inode
void makeDirectoryEntry(char* entryName, int inodeIndex, char* dirInode, FILE* disk){

  char inodeCharIndex = inodeIndex + '0';
  int entrySize = sizeof(entryName);

  //check the size of the entryName to be less than 31
  //if it is greater, print error message about max filename size/length
  if(entrySize < 32){

    int entryMade = 0; //0 is not made, 1 is made

    char* dirBlock = (char*)malloc(BLOCK_SIZE);
    char* entry = (char*)malloc(INODE_SIZE);
    short int blockNum;
    readInode(disk, inodeIndex, dirInode);

    for(int i = 0; i < 10; i++){

      if(entryMade == 1){
        break;
      }

      memcpy(&blockNum, dirInode + 8 + (i*2), 2);

      //if blockNum is 0, need to allocate a new block for this entry
      if(blockNum == 0){
        int* freeBlockIndex = findFreeBlocks(disk, 1, freeBlockIndex);

        blockNum = freeBlockIndex[0];
        updateFreeBlockVector(disk, blockNum, 1); //update the freeblockvector to indicate it has been occupied after writing to it
        memcpy(dirInode + 8 + (i*2), &blockNum, 2); //update the blocknumber on the correct inode index
      }

      readBlock(disk, blockNum, dirBlock);

      for(int j = 0; j < 15; j++){
        memcpy(entry, dirBlock + (j*INODE_SIZE), INODE_SIZE); //Copy the entry over from the block to a string
        //check if the string is empty, if yes then make the entry and exit the loops
        if(strlen(entry) == 0){
          memcpy(entry, &inodeCharIndex, 1); //Write the inode index as the first byte
          memcpy(entry + 1, entryName, strlen(entryName)); //Write the entry string

          //update the filesize for this inode and write it back
          memcpy(dirInode, &entrySize, sizeof(int));
          writeInode(disk, inodeIndex, dirInode);

          //Copy the entry to the block are write it back
          memcpy(dirBlock + (j*INODE_SIZE), entry, INODE_SIZE);
          writeBlock(disk, blockNum, dirBlock);

          entryMade = 1;

          break;
        }
      }
    }

    free(dirBlock);
    free(entry);
  }
  else{
    printf("Cannot create file as filename exceeds 30 characters. Please choose a shorter filename and try agian.");
    exit(0);
  }
}

int allocInodeForEmptyDir(char parentDirInodeIndex, char* dirName, FILE* disk){

  int index = findFreeInode(disk);  //find a free inode, return the index of the newly created inode
  int dirNameSize = sizeof(dirName);
  createInode(index, 0, dirNameSize, disk); //allocate an inode for the directory
  updateFreeInodeList(disk, index, 0x01); //update the free inode list

  int entryFound = 0; //0 is not found, 1 is found

  //Copy the dirName over to dirEntry with the 1st byte as 0 to match the entry format
  char c = 0;
  char* dirEntry = (char*)malloc(INODE_SIZE);
  memcpy(dirEntry, &c, 1);
  memcpy(dirEntry + 1, dirName, strlen(dirName));

  //Find the directory entry and update its inode index from 0 to the newly allocated index
  char* parentDirInode = (char*)malloc(INODE_SIZE);
  char* dirBlock = (char*)malloc(BLOCK_SIZE);
  char* entry = (char*)malloc(INODE_SIZE);
  short int blockNum;
  readInode(disk, parentDirInodeIndex, parentDirInode);

  for(int i = 0; i < 10; i++){

    if(entryFound == 1){
      break;
    }

    memcpy(&blockNum, parentDirInode + 8 + i*2, 2);
    readBlock(disk, blockNum, dirBlock);

    for(int j = 0; j < 15; j++){
      memcpy(entry, dirBlock + j*INODE_SIZE, INODE_SIZE); //Copy the entry over from the block to a string
      if(strcmp(dirEntry, entry) == 0){
          //Update the 1st byte and write the block back
          memcpy(entry, &index, 1);
          memcpy(dirBlock + j*INODE_SIZE, entry, INODE_SIZE);
          writeBlock(disk, blockNum, dirBlock);

          entryFound = 1;

          break;
      }
    }
  }

  free(dirEntry);
  free(parentDirInode);
  free(dirBlock);
  free(entry);

  return index;
}

void createFile(char* filename, char* filedata, char* path){

  //if creating file in empty dir, got to allocate an inode and update the inode number entry

  FILE* disk = fopen(vdisk_path, "rb+");

  int index = findFreeInode(disk);  //Will always be 1 or greater as inode 0 is for the root directory

  int filesize = sizeof(filedata);

  createInode(index, 1, filesize, disk);
  updateFreeInodeList(disk, index, 0x01);

  //Update the relevant directory inode with the new file entry
  char* dirInode = (char*)malloc(INODE_SIZE);
  int dirIndex;
  bool isRoot = false;

  int numTokens = 0;
  char* token1 = path;

  //Assign the string left after 2nd '/' to token, count number of tokens, repeat till the path is exhausted
  while((token1 = strchr(token1, '/')) != NULL){

    if(strcmp(token1, "/") == 0){
      break;
    }
    numTokens++;
    token1++;
  }

  //path passed is that of root directory
  if(numTokens == 0){
    dirIndex = 0;
    isRoot = true;
  }
  else{
    dirIndex = openDirectory(path, disk); //Inside here, if inode index comes to be 0, allocate an inode for it (it was called to read/write a file or subdir)
  }

  char* lastTokenOfPath = malloc(50); //arbitrary allocation size
  char* token2 = strrchr(path, '/');
  if(token2 != NULL){
    lastTokenOfPath = token2+1;
  }

  //if dir index is 0 and it is not the rootDirectory, makeDirectoryEntry as there in no inode for dir
  if((dirIndex == 0) && (isRoot == false)){
    dirIndex = allocInodeForEmptyDir(dirIndex, lastTokenOfPath, disk);
  }

  readInode(disk, dirIndex, dirInode);

  //convert the int index to char to be stored as the first byte of filename entry
  //char fileInodeIndex = index;

  //Write the filename to the blocks of this inode and save, with the first byte being index of the inode containing the filedata
  makeDirectoryEntry(filename, index, dirInode, disk); //make filename entry

  //free(lastTokenOfPath);      //this is cray, dont do it
  free(dirInode);

  //If filesize is greater than 0, read in the created file inode so that block indices can be added to it after writing the blocks
  //Dont write anything to allocated inode if size if 0 (unlike directories though, an inode is allocated for files even when empty)
  if(sizeof(filedata) > 0){

    char* inode = (char*)malloc(INODE_SIZE);
    readInode(disk, index, inode);
    int blockspan = (sizeof(filedata)/BLOCK_SIZE) + 1;

    //if block span greater than 10, reject the file
    if(blockspan > 10){
      printf("Cannot create requested file as filesize exceeds max filesize (5120 bytes).\n");
      exit(0);
    }

    int* freeBlocks = calloc(blockspan, 2);
    freeBlocks = findFreeBlocks(disk, blockspan, freeBlocks);

    char* dataChunk = (char*)malloc(BLOCK_SIZE); //Figure out how to chunk the data

    for(int i = 0; i < blockspan; i++){
      memcpy(dataChunk, &filedata[i*BLOCK_SIZE], BLOCK_SIZE); //copy over the next chunk of data to dataChunk
      writeBlock(disk, freeBlocks[i], dataChunk); //write the chunk to the block on index i of the freeBlocks array
      updateFreeBlockVector(disk, freeBlocks[i], 1);

      memcpy(inode + 8 + i, &freeBlocks[i], 2); //copy the blockIndex to the files inode
    }

    writeInode(disk, index, inode);  //Write back the inode with the updated block indices
    free(inode);
    free(freeBlocks);
  }

  fclose(disk);
}

void createDirectory(char* dirName, char* path){

  FILE* disk = fopen(vdisk_path, "rb+");

  //if creating dir in empty dir, got to allocate an inode and update the inode number entry
  char* dirInode = (char*)malloc(INODE_SIZE);
  char dirIndex;
  bool isRoot = false;

  int numTokens = 0;
  char* token1 = path;

  //Assign the string left after 2nd '/' to token, count number of tokens, repeat till the path is exhausted
  while((token1 = strchr(token1, '/')) != NULL){

    if(strcmp(token1, "/") == 0){
      break;
    }
    numTokens++;
    token1++;
  }

  //path passed is that of root directory
  if(numTokens == 0){
    dirIndex = 0;
    isRoot = true;
  }
  else{
    dirIndex = openDirectory(path, disk); //Inside here, if inode index comes to be 0, allocate an inode for it (it was called to read/write a file or subdir)
  }

  char* lastTokenOfPath = (char*)malloc(50); //arbitrary allocation size
  char* token2 = strrchr(path, '/');
  if(token2 != NULL){
    lastTokenOfPath = token2+1;
  }

  //if dir index is 0 and it is not the rootDirectory, makeDirectoryEntry as there in no inode for dir
  if(dirIndex == 0 && (isRoot == false)){
    dirIndex = allocInodeForEmptyDir(dirIndex, lastTokenOfPath, disk);
  }

  readInode(disk, dirIndex, dirInode);

  //do not allocate an inode or blocks for this new dir now, just write its name with 1st byte as 0
  //Write the directory name to the blocks of this inode and save, with the first byte 0 to signify that the dir is empty
  makeDirectoryEntry(dirName, 0, dirInode, disk);

  //free(lastTokenOfPath);  //this is cray, dont do it
  free(dirInode);

  fclose(disk);
}

char* readFile(char* filename, char* path){

  char* contents = "teststring";

  return contents;
}

void initLLFS(){

  //Create vdisk
  FILE* disk = fopen(vdisk_path, "wb");
  char* init = calloc(BLOCK_SIZE * NUM_BLOCKS, 1); //init allocated memory to all 0s
  fwrite(init, BLOCK_SIZE * NUM_BLOCKS, 1, disk); //write all 0s to disk
  free(init);
  fclose(disk);

  //Open disk to initialize superblock, the free block vector, and inodes for directories and files
  disk = fopen(vdisk_path, "rb+");

  initSuperBlock(disk); //Initialize superblock
  initFreeBlockVector(disk);  //Initialize free block vector
  initRootDirectory(disk);  //Initialize root directory

  fclose(disk);
}
