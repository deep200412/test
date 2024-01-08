/* Writing data into a mmaped file */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>

typedef struct student_s {
  char *name;
  int roll;
} student_t;

void init_student_data(student_t *s, const char*n, int r)
{
  s->name = strdup(n);
  s->roll = r;
}

int write_student_data(student_t *s, char *mmap_p)
{
  int str_size = strlen(s->name);
  memcpy(mmap_p, s->name, str_size+1);
  mmap_p += str_size+1;
  memcpy(mmap_p, &s->roll, sizeof(s->roll));
  return (str_size + 1 + sizeof(s->roll));
}

void main()
{
  const char *write_file_name = "tmp.db";
  int mode = 0x0777;
  
  int wfd = open(write_file_name, O_RDWR | O_CREAT| O_TRUNC, S_IRWXU);
  if (wfd == -1) {
    perror("Unable to open write file");
    return;
  }

  int MAX_BYTES = 1000;
  
  if (lseek(wfd, MAX_BYTES, SEEK_SET) == -1) {
    perror("Seek error");
    return;
  }

  if (write(wfd, "", 1) == -1) {
    perror("Write to file failed");
    return;
  }
  
  char *w_file_buf = mmap(0, MAX_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED, wfd, 0);
  if (w_file_buf == (caddr_t)-1) {
    perror("Unable to open mmap");
    return;
  }
  student_t s1;
  int num_bytes = 0;
  init_student_data(&s1, "abc", 10);
  num_bytes = write_student_data(&s1, w_file_buf);
  w_file_buf += num_bytes;
  init_student_data(&s1, "DEFGH", 750);
  num_bytes = write_student_data(&s1, w_file_buf);
  w_file_buf += num_bytes;
  init_student_data(&s1, "UVQ", 43);
  write_student_data(&s1, w_file_buf);
  puts("\n");
}