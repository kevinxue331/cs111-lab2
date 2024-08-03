#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  u32 remaining_time;
  //u32 starting_time;
  //bool finished;
  bool started;
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */
  bool finished = false;
  
 
  struct process *current;
  struct process *first;
  bool unordered = false;
  //check for the first process start time
  u32 start_time = data[0].arrival_time;;
  for(int i=0; i<size; i++){
    current= &data[i];
    data[i].remaining_time = data[i].burst_time;
    data[i].started = false;
    if(data[i].arrival_time < start_time){
      start_time=data[i].arrival_time;
      current = &data[i];
      unordered = true;
    }
  }
  if(unordered){
    for(int i=0;i<size;i++){
      if(data[i].arrival_time==start_time){
        current = &data[i];
        break;
      }
    }
  }
  
  u32 slice_time=1;
  u32 total_time =start_time;
  //printf("%d\n",current->arrival_time);
  //current=&data[size-1];
  //struct process *new;
  if (quantum_length == 0) finished = true;

  while(finished==false){
    //first add new tasks before finishing current task
    for (int i=0; i<size; i++){
      if (data[i].arrival_time ==total_time) TAILQ_INSERT_TAIL(&list, &data[i], pointers);
    }
    //after the slice is over add the process back to the queue
    if(slice_time==quantum_length+1&&current->remaining_time>0){
      TAILQ_INSERT_TAIL(&list, current, pointers);
      slice_time= 1;
    }
    // at the start of each slice pull the first and remove it from the queue to run
    if(slice_time==1){
      if (TAILQ_EMPTY(&list)) {
        total_time++;
        continue;
      }
      current=TAILQ_FIRST(&list);
      TAILQ_REMOVE(&list, current, pointers);
    }
    //calculate the response time for each process when it first runs
    if (!current->started) {
          total_response_time = total_response_time + total_time - current->arrival_time;
          current->started = true;
    }
    //if slice is still running
    if(slice_time<quantum_length+1){
      if (current->remaining_time > 0) {
          current->remaining_time = current->remaining_time - 1;
          total_time++;
      }
     //if task if over calculate the waiting time fo
      if (current->remaining_time == 0) {
        total_waiting_time = total_waiting_time + total_time - current->arrival_time - current->burst_time;          
        slice_time = 0;
      }
      
    }
    //start_time++;
    slice_time++;
    finished = true;
    
      for(int i=0;i<size;i++){
       //printf("%d",data[i].remaining_time);
        if(data[i].remaining_time>0){
          finished = false;
          break;
        }
      }
    
  //finished = true;
  }
  /* End of "Your code here" */
  
  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
