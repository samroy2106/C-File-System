//Demonstrates the creation of a 2 files in the root directory, the first of which is then read from the vdisk

#include <stdio.h>
#include "../io/File.h"

int main(int argc, char* argv[]){

  printf("Creating files in the root directory...\n");

  //Format the disk for use
  initLLFS();

  //File contents. In this case strings. Can be anything though.
  char* contents1 = "This is a test that demonstrates the successful creation of a file in the root directory.";
  char* contents2 = "Creating another file in the root directory...";

  //create files in the root dircetory
  createFile("testfile1", contents1, "/");  //filename, file contents, path
  createFile("testfile2", contents2, "/");

  //readFile("testfile1, "/"); //read the contents of the one of the files and print to console
}
