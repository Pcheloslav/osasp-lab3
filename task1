#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

int ProcessChild(pid_t ChildPid);
void GetTime(char* Caller);
int WaitChild(pid_t ChildPid);

int main()
{
  pid_t ChildPid1 = 0;
  ChildPid1 = fork();
  if (ProcessChild(ChildPid1) > 0){
    pid_t ChildPid2 = 0;
    ChildPid2 = fork();
    int iResult = ProcessChild(ChildPid2);
    if (iResult == -1){
      if (WaitChild(ChildPid1) == 1){
        return 2;
      }
    }else if (iResult > 0){
      GetTime("Parent");
      if (system("ps -x") == -1){
        perror("Cannot show information about active processes\n");
      }
      if (WaitChild(ChildPid1) == 1){
        return 3;
      }
      if (WaitChild(ChildPid2) == 1){
        return 4;
      }
    }
  }
  return 0;
}

int ProcessChild(pid_t ChildPid)
{
  switch (ChildPid){
    case -1:
			perror("Child 1 process could not be created\n");
      return -1;
			break;

    case 0:
      GetTime("Child 1");
      return 0;
      break;
  }
  return 1;
}

void GetTime(char* Caller){
	struct timeval TV;
	
	if (gettimeofday(&TV, NULL) == -1)
		perror("Can not get current time\n");
	else {
		int MSec = TV.tv_usec / 1000;
		int Sec = TV.tv_sec % 60;
		int Min = (TV.tv_sec / 60) % 60;
		int Hrs = (TV.tv_sec / 60 * 60 + 3) % 24;
	
		printf("Caller: %s; Pid: %d Parrent Pid: %d;\nTime: %02d:%02d:%02d:%03d\n\n", Caller, getpid(), getppid(), Hrs, Min, Sec, MSec);	
	}
}

int WaitChild(pid_t ChildPid) {
	if (waitpid(ChildPid, NULL, 0) == -1)
		perror("Wait pid failue\n");
}
