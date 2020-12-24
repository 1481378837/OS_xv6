#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  int i, j;
  int pid;
  int len;
  int blank; // use int for bool
  char args[MAXARG][32]; //each arg length limited within 32
  char *p[MAXARG];
  char buf;

  if(argc < 2) 
  {
    fprintf(2, "plz input like: xargs cmd  ...\n");
    exit();
  }
  while (1)// parent process always monitors
  {
    i = 0;
    blank = 0;
    memset(args, 0, MAXARG * 32); //init args[]

    for(j = 1; j < argc; j++) // argv[0] should be ignored
    {
      strcpy(args[i], argv[j]);//copy command and stdin args together
      i++;
    }
    j = 0;  
    while (i < MAXARG-1) 
    {
      if ((len = read(0, &buf, 1)) <= 0) // get from stdin
      {
        // CTRL+D 
        wait(); 
        exit();
      }
      if (buf == '\n') //arg end
      {
        break;
      }
      if (buf == ' ') // blank -> next arg
      {
        if (blank) 
        {
          i++;
          j = 0;
          blank = 0;
        }
        continue;
      }
      args[i][j] = buf; //get one char
      j++;
      blank = 1;
    }
    for (i = 0; i < MAXARG-1; i++) 
    {
      p[i] = args[i];// let p[i] point to args[i]
    }
    // let last arg be 0
    p[MAXARG-1] = 0; 
    
    // exec command
    if ((pid = fork()) == 0) 
    {
      exec(argv[1], p);//char* argv[]
      exit();
    }
  }
  exit();
}