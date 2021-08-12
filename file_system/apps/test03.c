//Demonstrates the creation of 2 subdirectories in the root directory, one of then futher containing a sub directory

#include <stdio.h>
#include "../io/File.h"

int main(int argc, char* argv[]){

  printf("Creating sub directories in root and beyond...\n");

  //Format the disk for use
  initLLFS();

  createDirectory("home", "/");
  createDirectory("work", "/");

  createDirectory("schoolwork", "/work");

  //There is not really a good way to test with without trying to open a file as a new directory is
  //essentially just a 32 byte entry in one of the blocks pointed to by the parent directories inode.
  //It does not have an inode allocated for it till the time an entry (file or directory) needs to be
  //made in it.
  //Even though this test demonstrates the creation of multiple sub directories, its results need to be
  //be viewed in a hex editor in the relavant block. So best to open a file in a subdirectory to demostrate. (test04) 
}
