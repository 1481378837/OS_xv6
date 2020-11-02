#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *re) 
{
  // refer to ls.c
  char buf[512], *p;
  int fd;
  struct dirent dire;
  struct stat st;
  if((fd = open(path, 0)) < 0)//open_err
  {
    fprintf(2, "cannot open %s\n", path);
    return;
  }
  if(fstat(fd, &st) < 0 || st.type != T_DIR) //not DIR
  {
    fprintf(2, "the first arg must be dir path\n");
    close(fd);
    return;
  }

  while(read(fd, &dire, sizeof(dire)) == sizeof(dire)) //sub-dir
  {
    if(dire.inum == 0) 
    {
      continue;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p = '/';
    p++;
    memmove(p, dire.name, DIRSIZ);
    p[DIRSIZ] = 0;  //add '0' end
    if(stat(buf, &st) < 0)
    {
      printf("cannot stat %s\n", buf);
      continue;
    }
    if(st.type == T_FILE)
    {
      if(strcmp(re, dire.name)== 0)
      {
        printf("%s\n", buf); //reach end
      }
    }
    else if(st.type == T_DIR)
    {
      if(strcmp(dire.name, ".")!=0 && strcmp(dire.name, "..")!=0)
      {
        find(buf,re); //recursion
      }
    }
  }
  close(fd);
}

int main(int argc, char *argv[])
{
  if(argc < 3)
  {
    fprintf(2, "plz input like: find path expression\n");
    exit();
  }
  find(argv[1], argv[2]);
  exit();
}