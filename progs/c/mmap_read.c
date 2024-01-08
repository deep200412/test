/* Reading data from a mmaped file */

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

int read_student_data(student_t *s, char *mmap_p)
{
  char name_ar[100];
  int str_size = 0;
  int i = 0;
  for(i=0; i<100; i++) {
    if (mmap_p[i] == '\0') {
      break;
    }
    name_ar[i] = mmap_p[i];
  }
  str_size = i;
  name_ar[i] = '\0';
  s->name = name_ar;
  s->roll = *((int *)(mmap_p+i+1));
  return (str_size + 1 + sizeof(s->roll));
}

void main()
{
  const char *read_file_name = "tmp.db";
  int mode = 0x0777;
  
  int fd = open(read_file_name, O_RDWR);
  if (fd == -1) {
    perror("Unable to open read file");
    return;
  }

  int MAX_BYTES = 1000;
  
  char *file_buf = mmap(0, MAX_BYTES, PROT_READ, MAP_SHARED, fd, 0);
  if (file_buf == (caddr_t)-1) {
    perror("Unable to open mmap");
    return;
  }
  student_t s1;
  int num_bytes = 0;
  num_bytes = read_student_data(&s1, file_buf);
  file_buf += num_bytes;
  printf("Student name = %s, roll = %d\n",s1.name, s1.roll);
  num_bytes = read_student_data(&s1, file_buf);
  file_buf += num_bytes;
  printf("Student name = %s, roll = %d\n",s1.name, s1.roll);
  read_student_data(&s1, file_buf);
  printf("Student name = %s, roll = %d\n",s1.name, s1.roll);
  puts("\n");
}
