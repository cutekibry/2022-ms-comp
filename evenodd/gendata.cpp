#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

void usage() {
  printf("usage: ./gendata <file_bytes> <file_name>\n");
  printf("       ./gendata <file_bytes> <file_name> <seed>\n");
}

const int SZ = 1 << 20;

FILE *file;
char buf[SZ];
char *st = buf, *ed = buf + SZ;

void flush() {
  fwrite(buf, 1, st - buf, file);
  st = buf;
}
void put(char ch) {
  if(st == ed) 
    flush();
  (*st) = ch;
  st++;
}

long long time_ms() {
  timeval time;
  gettimeofday(&time, NULL);
  return time.tv_sec * 1000 + time.tv_usec / 1000;
}

long long atoi64(const char *s) {
  long long result = 0;
  int i;
  int length = strlen(s);

  for (int i = 0; i < length; i++)
      result = result * 10 + (s[i] - '0');
  return result;
}

void file_create(const char *file_name) {
  int n = strlen(file_name);
  char str[n + 1];
  struct stat tmp_stat;

  for (int i = 0; i < n; i++) {
    str[i] = file_name[i];
    if (str[i] == '/') {
      str[i] = '\0';
      if (access(str, 0) == -1)
        mkdir(str, 0777);
      str[i] = '/';
    }
  }
  str[n] = '\0';
  fclose(fopen(str, "wb"));
}

int main(int argc, char **argv) {
  if(argc < 3 or argc > 4) {
    usage();
    return -1;
  }

  file_create(argv[2]);

  long long file_bytes = atoi64(argv[1]);
  file = fopen(argv[2], "wb");

  if(argc == 4)
    srand(atoi(argv[3]));
  else
    srand(time_ms());

  for (long long i = 0; i < file_bytes; i++)
    put(rand() & 255);
  flush();
  fclose(file);
}