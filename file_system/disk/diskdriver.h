#ifndef DISKDRIVER_H
#define DISKDRIVER_H

//Reads one block at a time from disk
void readBlock(FILE* disk, int blockNum, char* buffer);

//Writes one block at a time to disk
void writeBlock(FILE* disk, int blockNum, char* data);

void readInode(FILE* disk, int blockNum, char* buffer);

void writeInode(FILE* disk, int blockNum, char* data);

#endif /* DISKDRIVER_H */
