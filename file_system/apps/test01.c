//Demostrates the initiaization of the file system on vdisk
//Please read the vdisk in a hex editor to read the superblock, free block vector and free inode list contents

#include <stdio.h>
#include "../io/File.h"

int main(int argc, char* argv[]){

  printf("Initiating vdisk with superblock, free block vector, free inode list and root directory inode info...\n");
  initLLFS();
}
