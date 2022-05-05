#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<dirent.h>
#include<fcntl.h>

#define DT_REG 8
#define DefaultDir "/bin/"
#define _countof(array) (sizeof(array) / sizeof(array[0]))

#define RHFileMode S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#define RHFileFlags O_CREAT
int RedirectHandler(const char *FileName, int Handle, int AddMode);
int RedirectProcess(char *Arguments[], int AmOfArg);
int GetCommandList(struct dirent ***Commands); 
int GetNextCommand(char *argv[], int argc, int *FirsArg, int* CurArg, struct dirent **Commands, int AmOfCommands);
int ExecuteCommand(char **Directory, char *Arguments[]);
int WaitChild(pid_t ChildPid);

int main(int argc, char* argv[])
{
  if (argc < 2){
    fprintf(stderr, "Incorrect parameters: ./{FileName} {Command}\n{FileName}: Name of exec. file\n{Command}: Command to exec.\n");
    return 1;
  }
  struct dirent **Commands;
  int AmOfCommands = GetCommandList(&Commands);
  int CurArg = 1, FirstArg = 1;
  while (CurArg < argc){
    if (GetNextCommand(argv, argc, &FirstArg, &CurArg, Commands, AmOfCommands) != 0){
      fprintf(stderr, "Could not determine commnad\n");
      return 2;
    }

    char *Arguments[CurArg - FirstArg + 1];
    for (int i = 0; i < CurArg - FirstArg; i++){
      Arguments[i] = argv[i + FirstArg]; 
    }
    Arguments[CurArg - FirstArg] = NULL;

    char *Directory = calloc(_SC_TRACE_NAME_MAX, sizeof(char));
    if (Directory == NULL){
      perror("Could not allocate memory\n");
      return 3;
    }
    strcat(Directory, DefaultDir);
    strcat(Directory, Arguments[0]);
    ExecuteCommand(&Directory, Arguments); 
    
  
    free(Directory);
  } 

  
  for (int i = 0; i < AmOfCommands; i ++){
    free(Commands[i]);
  }
  free(Commands);
  return 0;
}

int RedirectHandler(const char *FileName, int Handle, int AddMode)
{
  int Fd = open(FileName, RHFileFlags | AddMode, RHFileMode);
  if (Fd == -1){
    fprintf(stderr, "Could not open file \"%s\"\n", FileName);
    return -1;
  }
  if (dup2(Fd, Handle) < 0) {
        close(Fd);
        fprintf(stderr, "Could not override handle \"%d\"\n", Handle);
        return -2;
    }
  close(Fd);
  return Handle;
}

int RedirectProcess(char *Arguments[], int AmOfArg)
{
  int Offset = 0;
  int Result = -3;
  if (strstr(Arguments[AmOfArg-1], ">>") ||
      strstr(Arguments[AmOfArg-2], ">>")){
      if (Arguments[AmOfArg-1][0] == '>'){
        Offset = 2;
      }
      Result = RedirectHandler(&(Arguments[AmOfArg-1][Offset]), STDOUT_FILENO, O_WRONLY | O_APPEND);  
  }

  if (strstr(Arguments[AmOfArg-1], ">") ||
      strstr(Arguments[AmOfArg-2], ">")){
      if (Arguments[AmOfArg-1][0] == '>'){
        Offset = 1;
      }
      Result = RedirectHandler(&(Arguments[AmOfArg-1][Offset]), STDOUT_FILENO, O_WRONLY | O_TRUNC);  
  }

  if (strstr(Arguments[AmOfArg-1], "<") ||
      strstr(Arguments[AmOfArg-2], "<")){
      if (Arguments[AmOfArg-1][0] == '<'){
        Offset = 1;
      }
      Result = RedirectHandler(&(Arguments[AmOfArg-1][Offset]), STDIN_FILENO, O_RDONLY);  
  }

  if (Result > 0){
    if (Offset == 0){
      Arguments[AmOfArg-2] = NULL;
    }else{
      Arguments[AmOfArg-1] = NULL;
    }
  }
  return Result;
}

int ScanDirFilter(const struct dirent *FilePtr);

int GetCommandList(struct dirent ***Commands)
{
  int iResult = scandir("/bin", Commands, ScanDirFilter, NULL);
  if (iResult == -1){
    perror("Could not get commands list");
  }
  return iResult;
}

int ScanDirFilter(const struct dirent *FilePtr)
{
  if (FilePtr->d_type == DT_REG){ 
    return 1;
  }
  return 0;
}

int GetNextCommand(char *argv[], int argc, int *FirstArg, int* CurArg, struct dirent **Commands, int AmOfCommands)
{
  int AmOfArg = 2, CurArg = 0;
  int iResult = 0;
  while (iResult == 0 && *CurArg < argc){
    for (int j = 0; j < AmOfCommands && iResult == 0; j++){
      if (strcmp(argv[*CurArg], Commands[j]->d_name) == 0){
        iResult = 1;
      }
    }
    if (iResult == 1){
      *FirstArg = *CurArg;
    }
    (*CurArg)++;
  }
  if (iResult == 0 && *CurArg == argc){
    fprintf(stderr, "Could not find command\n");
    return 1;  
  }

  iResult = 0;
  while (iResult == 0 && *CurArg < argc){
    for (int j = 0; j < AmOfCommands && iResult == 0; j++){
      if (strcmp(argv[*CurArg], Commands[j]->d_name) == 0){
        iResult = 1;
      }
    } 
    (*CurArg)++;
  }
  if (iResult == 1){
    (*CurArg)--;
  }
  return 0;
}

int ExecuteCommand(char **Directory, char *Arguments[])
{
  int AmOfArg = 0;
  while (Arguments[AmOfArg] != NULL) AmOfArg++;
  int ThStdIn = dup(STDIN_FILENO);
  int ThStdOut = dup(STDIN_FILENO);
  int iResult = RedirectProcess(Arguments, AmOfArg);
  if (iResult <= 0 && iResult != -3){
    close(ThStdIn);
    close(ThStdOut);
    return 1;
  }

  
  pid_t ChildPid = fork();
  switch (ChildPid){
    case -1:
			perror("Child 1 process could not be created\n");
      return 1;
      break;
    case 0:
      if (execvp(*Directory, Arguments) == -1){
        perror("Execute command failue");
        return 2;
      }
      break;
    default:
      if (WaitChild(ChildPid) == 1){
        return 1;
      }
      if (iResult != -3){
        switch(iResult){
          case 0:
            iResult = dup2(ThStdIn, STDIN_FILENO);
            break;
          case 1:
            iResult = dup2(ThStdOut, STDOUT_FILENO);
            break;
        } 
        if (iResult < 0){
          close(ThStdIn);
          close(ThStdOut);
          fprintf(stderr, "Could not return overrited handle\n");
          return 3;
        }
      }
      close(ThStdIn);
      close(ThStdOut);
      break;
  }
}

int WaitChild(pid_t ChildPid) 
{
	if (waitpid(ChildPid, NULL, 0) == -1)
		perror("Wait pid failue\n");
}
