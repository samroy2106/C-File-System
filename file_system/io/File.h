#ifndef FILE_H
#define FILE_H

void initLLFS(void);
void createFile(char* filename, char* filedata, char* path);
void createDirectory(char* dirName, char* path);
//char* readFile(char* filename, char* path);

#endif /* FILE_H */
