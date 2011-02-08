#include <user/sys_misc.h>
#include <user/sys_fio.h>
#include <stdio.h>

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    printf("Usage: rm files...\n");
    return 0;
  }

  for(i = 1; i < argc; i++){
    if(unlink(argv[i]) < 0){
      printf("rm: %s failed to delete\n", argv[i]);
      break;
    }
  }

  return 0;
}

