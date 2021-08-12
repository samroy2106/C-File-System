//Creates files in directories other the root directory, reading one of them to then demostrate the proper creation of subdirectories and files within them

#include <stdio.h>
#include "../io/File.h"

int main(int argc, char* argv[]){

  printf("Creating files in subdirectories then reading one of them...\n");

  //Format the disk for use
  initLLFS();

  //File contents. In this case, a string. Can be anything though.
  char* contents1 = "This is a test that demonstrates the successful creation of a file in the root directory.";
  char* contents2 = "This is a test that demonstrates the successful creation of a file in the root directory.";
  char* contents3 = "This is a test that demonstrates the successful creation of a file in the root directory.";

  createDirectory("home", "/");
  createDirectory("work", "/");

  createDirectory("schoolwork", "/work");

  createFile("filename1", contents1, "/home");
  createFile("filename2", contents2, "/work");
  createFile("filename3", contents3, "/work/schoolwork");

  //readFile("filename3, "/work/schoolwork"); //read the contents of the one of the files and print to console
}
