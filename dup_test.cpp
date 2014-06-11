#include <unistd.h>
#include <cstdio>
#include <fcntl.h>
#include <stdlib.h>

#include <map>
#include <vector>

struct pipe_t {
  pipe_t() {
    fds[0] = -1;
    fds[1] = -1;
  }
  int init() {
    return pipe(fds);
  }
  ~pipe_t() {
    if (fds[0] != -1) {close(fds[0]); fds[0] = -1; }
    if (fds[1] != -1) {close(fds[1]); fds[1] = -1; }
  }
  int fds[2];
};

#define MAX(a, b) ((a) > (b) ? (a) : (b))

template <typename action_t>
int process_fd(int *myfds, int fd_count, action_t &action) {
  int maxfd = 0;
  fd_set readset;
  // Initialize the set
  FD_ZERO(&readset);
  for (int j = 0; j < fd_count; ++j) {
    if (myfds[j] == -1) continue;
    FD_SET(myfds[j], &readset);
    maxfd = MAX(maxfd, myfds[j]);
  }

  printf("select...\n");
  // Now, check for readability
  int result = select(maxfd + 1, &readset, NULL, NULL, NULL);
  printf(" ... %d\n", result);
  if (result == -1) {
    // Some error...
    return -1;
  }
  else {
    for (int j = 0; j < fd_count; ++j) {
      if (myfds[j] == -1) continue;
      if (FD_ISSET(myfds[j], &readset)) {
        // myfds[j] is readable
        printf("action on %d\n", myfds[j]);
        int res = action.on_ready(myfds[j]);
        printf("...action on %d -> %d\n", myfds[j], res);
        if (res == -1) { 
          return -1;
        } 
      }
    }
  }
  return 0;
}

struct fd_handler_base_t {
  virtual int on_eof() = 0;
  virtual int on_read(void* data, ssize_t sz) = 0;
};

struct callback_handler_t {
  callback_handler_t(void *_buff, size_t _sz)
    :buff(_buff)
    ,buff_size(_sz) {}

  int on_ready(int &fd) {
    printf("read from %d...\n", fd);
    ssize_t read_sz = read(fd, buff, buff_size);
    printf("%zu bytes read from %d\n", read_sz, fd);
    if (read_sz == -1) {
      // error
      return -1;
    } else if (read_sz == 0) {
      // eof
      fd_map_t::iterator it = fd_map.find(fd);
      if (it == fd_map.end()) return -1;
      int res =  it->second->on_eof();
      close(fd);
      fd = -1; // close fd
      return res;
    } else {
      // do process
      fd_map_t::iterator it = fd_map.find(fd);
      if (it == fd_map.end()) return -1;
      return it->second->on_read(buff, read_sz);
    } 
  }
  typedef std::map<int, fd_handler_base_t*> fd_map_t;
  fd_map_t fd_map;
  void *buff;
  size_t buff_size;
};

struct child_t {
  child_t(int _fd) :fd(_fd) {}
  int fd;
};

struct cin_handler_t : public fd_handler_base_t {
  typedef std::vector<child_t>child_list_t;
  cin_handler_t()
    :child_count(0)
    ,last_child_id(0)
  {}
  cin_handler_t(int *_fd_list, int _fd_count) 
    :child_count(_fd_count)
    ,last_child_id(0)
  {
    for (int i = 0; i < _fd_count; ++i) {
      child_list.push_back(child_t(_fd_list[i]));
    }
  }

  virtual int on_eof() {
    for (size_t i = 0; i < child_count; ++i) {
      child_t &child = child_list[i];
      close(child.fd);
      printf("close child fd %d\n", child.fd);
    }
    return 0;
  }
  virtual int on_read(void* data, ssize_t sz) {
    // line per child process
    size_t child_id = last_child_id;
    char* last_it = (char*)data;
    char* end = &((char*)data)[sz];

    for(;; child_id = (child_id + 1) % child_count) {
      child_t &child = child_list[child_id]; 
      char* it = (char*)memchr((void*)last_it, '\n', sz);
      
      size_t line_size = it ? it + 1 - last_it : end - last_it;
      if (!line_size) break;
      ssize_t write_sz = write(child.fd, last_it, line_size);
      printf("write %zu (%zu) bytes to child fd %d\n", line_size, write_sz, child.fd);
      if (!it) break;
      last_it = it + 1;
    }
    last_child_id = child_id;
    printf("..on read\n");
    return sz;
  }
  child_list_t child_list;
  size_t child_count;
  size_t last_child_id; 
};

void* memrchr(void *data, char ch, int sz) {
  for (char *it = sz + (char*)data; it != (char*)data; --it) {
    if (*it == ch) return (void*)it;
  }
  return NULL;
}
struct child_handler_t : public fd_handler_base_t {
  child_handler_t(int _fd_out) :fd_out(_fd_out), pfx_data(NULL), pfx_sz(0), skip_flag(false) {}

  virtual int on_eof() {
    if (!skip_flag) write(fd_out, pfx_data, pfx_sz);
    clear_pfx();
    return 0;
  }
  int set_pfx(void* data, size_t sz) {
    clear_pfx();
    pfx_data = malloc(sz);
    if (pfx_data == NULL) return -1;
    pfx_sz = sz;
    memcpy(pfx_data, data, sz);
    return 0;
  }
  virtual int on_read(void* data, ssize_t sz) {
    void *it = memchr(data,'\n', sz);
    void *it_last = it ? memrchr((void*)(1 + (char*)it), '\n', sz) : NULL;

    printf("on_read on child ... \n"); 
    if (!it) {
      // skip long lines
      if (pfx_sz) skip_flag = true;

      if (skip_flag) {
        clear_pfx();
      } else {
        if(-1 == set_pfx(data, sz)) {
          return -1;
        }
      }
      return 0;
    } else {
      if (!skip_flag) {
        write(fd_out, pfx_data, pfx_sz);
        clear_pfx();
        size_t len = (char*)it_last - (char*)data;
        write(fd_out, data, len);
        
        pfx_data = malloc(sz - len);
        if (pfx_data) {
          pfx_sz = sz - len;
          memcpy(pfx_data, it_last, pfx_sz);
        } else {
          return -1;
        }
      } else {
        clear_pfx();
        skip_flag = false;
        write(fd_out, it, (char*)it_last - (char*)it);
        printf("write %zu bytes to %d\n", (char*)it_last - (char*)it, fd_out); 
        size_t len = (char*)it_last - (char*)data;
        pfx_data = malloc(sz - len);
        if (pfx_data) {
          pfx_sz = sz - len;
          memcpy(pfx_data, it_last, pfx_sz);
        } else {
          return -1;
        }
      }
      return 0;
    }
  }
  void clear_pfx() {
    free(pfx_data);
    pfx_data = 0;
    pfx_sz = 0;
  }
  int fd_out;
  void *pfx_data;
  size_t pfx_sz;
  bool skip_flag;
};

int split_input(int in_fd, int out_fd, int *child_in_fds, int *child_out_fds, int child_count) {
  int buff_size = 1024 * 32;
  void *buff = malloc(buff_size);
  callback_handler_t callback_handler(buff, buff_size);
  std::vector<int> fds;
  callback_handler.fd_map[in_fd] = new cin_handler_t(child_in_fds, child_count);
  fds.push_back(in_fd);
  for (int i = 0; i < child_count; ++i) {
    callback_handler.fd_map[child_out_fds[i]] = new child_handler_t(out_fd);
    fds.push_back(child_out_fds[i]);
  }
  while(true) {
    printf("process_fds ... \n");
    int res = process_fd(&fds[0], fds.size(), callback_handler);
    printf("... process fds %d\n", res);
    if (res == -1) {
      free(buff);
      return -1;
    }
    
    int closed_cnt = 0;
    for(int i = 0; i < fds.size(); ++i) {
      if(fds[i] == -1) ++closed_cnt;
    }
    if (closed_cnt == fds.size()) {
      free(buff);
      return 0;
    }
  }
  free(buff);
  return 1;
}

int main() {

  pipe_t in_pipe;
  if (-1 == in_pipe.init()) return 1;
  pipe_t out_pipe;
  if (-1 == out_pipe.init()) return 1;
  
  int pid = fork();
  if (pid) { // parent
    if(-1 == close(in_pipe.fds[0])) {fprintf(stderr,"Can't close %d\n", in_pipe.fds[0]); return 1;}
    if(-1 == close(out_pipe.fds[1])) {fprintf(stderr,"Can't close %d\n", out_pipe.fds[1]); return 1;}

    printf("child pid: %d\n", pid); 
    int child_in_fds[] = {in_pipe.fds[1]};
    int child_out_fds[] = {out_pipe.fds[0]};
    printf("split_input...\n");
    int res = split_input(0, 1, child_in_fds, child_out_fds, 1);
    printf("... split_input %d\n", res);
    close(out_pipe.fds[0]); in_pipe.fds[0] = -1; 
    return res==0 ? 0 : 1;
  } else { // child
    if(-1 == close(in_pipe.fds[1])) {fprintf(stderr,"Can't close %d\n", in_pipe.fds[1]); return 1;}
    if(-1 == close(out_pipe.fds[0])) {fprintf(stderr,"Can't close %d\n", out_pipe.fds[0]); return 1;}
    if(-1 == dup2(in_pipe.fds[0], 0)) {fprintf(stderr,"Can't dup 0\n"); return 1;}
    if(-1 == dup2(out_pipe.fds[1], 1)) {fprintf(stderr,"Can't dup 1\n"); return 1;}
    
    int return_status =  system("cat");
    /*
    fprintf(stderr, "CHILD: running child...\n");     
    size_t buff_len = 1024 * 32;
    char *buff = new char[buff_len];
    while(true) {
      fprintf(stderr, "CHILD: reading ...\n"); 
      ssize_t read_sz = read(0, (void*)buff, buff_len);
      if (read_sz == 0) {
        fprintf(stderr, "CHILD: 0 bytes read from %d\n", 0);
        break; // eof
      } else if (read_sz == -1) {
        fprintf(stderr, "CHILD: error on read\n");
        return 1; // error
      } else {
        fprintf(stderr, "CHILD: %zu bytes read from %d\n", read_sz, 0);
        write(1, (void*)buff, read_sz);
      }
    }
    */
    //close(in_pipe.fds[0]);
    //close(out_pipe.fds[1]);
    close(0);
    close(1);    
    return return_status;
  }

  return 0;
}
