
#include "../config.h"
#include <stdio.h>
#include "neo_misc.h"
#include "neo_err.h"
#include "ulist.h"
#include "neo_files.h"

int main(int argc, char **argv)
{
  char *path;
  ULIST *files = NULL;
  char *filename;
  NEOERR *err;
  int x;

  if (argc > 1)
    path = argv[1];
  else
    path = ".";

  ne_warn("Testing ne_listdir()");
  err = ne_listdir(path, &files);
  if (err)
  {
    nerr_log_error(err);
    return -1;
  }

  for (x = 0; x < uListLength(files); x++)
  {
    err = uListGet(files, x, (void *)&filename);
    printf("%s\n", filename);
  }

  uListDestroy(&files, ULIST_FREE);

  ne_warn("Testing ne_listdir_match() with *.c");
  err = ne_listdir_match(path, &files, "*.c");
  if (err)
  {
    nerr_log_error(err);
    return -1;
  }

  for (x = 0; x < uListLength(files); x++)
  {
    err = uListGet(files, x, (void *)&filename);
    printf("%s\n", filename);
  }

  uListDestroy(&files, ULIST_FREE);
  return 0;
}
