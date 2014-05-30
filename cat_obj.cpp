#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <inttypes.h>

template <typename data_t> 
struct shared_array {
  shared_array(char *_data) :data(_data) {}
  ~shared_array() {
    delete [] data;
  } 

  inline data_t *get() { return data; }
  inline const data_t *get() const { return data; }
  
  data_t *data;
};

struct slice_t {
  slice_t(char *_data, size_t _sz): begin(_data), sz(_sz) {}
  slice_t(): sz(0) {}

  size_t sz;
  char *begin;
};

struct write_buff_t {
  write_buff_t(FILE *_fd, char *_data, size_t _sz)
    : fd(_fd)
    , data(_data)
    , sz(_sz) {}

  FILE *fd;
  size_t sz;
  size_t offset;
  char *data;
};

inline size_t fwrite(void *data, size_t szof, size_t sz, write_buff_t &write_buff) {
  sz *= szof;
  if (write_buff.sz < write_buff.offset + sz) {
    fwrite(write_buff.data, 1, write_buff.offset, write_buff.fd);
    write_buff.offset = 0;

    if (sz > write_buff.sz) {
      fwrite(data, 1, sz, write_buff.fd);
    } else {
      memcpy(&write_buff.data[write_buff.offset], data, sz);
      write_buff.offset += sz;
    }
  } else {
    memcpy(&write_buff.data[write_buff.offset], data, sz);
    write_buff.offset += sz;
  }
  return 0;
}

void sync(write_buff_t &write_buff) {
  fwrite(write_buff.data, 1, write_buff.offset, write_buff.fd);
  write_buff.offset = 0;
}

template <typename file_t>
struct add_slice_t {
  add_slice_t(file_t _fd) : fd(_fd), count(0) {}

  inline void operator()(const slice_t &slice) {
    fwrite(slice.begin, 1, slice.sz, fd); 
  }

  inline size_t mem_used() const {
    return count * sizeof(slice_t);
  }

public:
  file_t fd;
  
  size_t count;
  slice_t *headers;
};

template <typename file_t>
struct print_out_t {
  print_out_t(file_t _fd) : fd(_fd) {}
  inline void operator()(char *data, size_t sz) const {
    // write to fd
    fwrite(data, 1, sz, fd); 
  }
  inline void commit() { //call it on the end of line to reset line context
    // nope
  }

  file_t fd;
};

template <typename slice_action_t , typename long_action_t>
bool process_stream(FILE *fd, char *buff,  size_t buff_sz, slice_action_t slice_action, long_action_t &long_action) {
  bool has_prefix = false;
  size_t offset = 0;

  while(!feof(fd)) {
    size_t read_sz = fread(&buff[offset], 1, buff_sz - offset, fd);
    read_sz += offset;
    offset = 0;
//    printf ("[%zu %zu %d %d]", buff_sz, read_sz, ferror(fd), feof(fd));
    if (ferror(fd)) {
      return false;
    }
    
    char *it = buff;
    if (has_prefix) { // call 
      char *new_it = (char*)memchr(buff, '\n', read_sz);
      if (new_it) {
        ++new_it;
        long_action(it, new_it - it);
        long_action.commit();
        has_prefix = false;
        it = new_it;
      } else {
        long_action(it, read_sz - (it - buff));
        continue;
      }
    }
    {
      while (it != buff + read_sz) {
        char * new_it = (char*)memchr(it, '\n', read_sz - (it - buff));
        if (new_it) {
          slice_t slice(it, new_it - it + 1);
          slice_action(slice);
          it = new_it + 1;
        } else {
          break;
        }
      }
      size_t sfx_len = (buff + read_sz) - it;
      if (sfx_len* 2 > buff_sz) {
        memmove(buff, it, sfx_len);
        offset = sfx_len;
      } else if (sfx_len > 0) {
        long_action(it, sfx_len);
        has_prefix = true; 
      }
    }
  }
  return true;
}


int main() {
  size_t buff_size  = 1024 * 64;
  shared_array<char> buff_data(new char[buff_size]);
  
  size_t w_buff_size  = 1024 * 32;
  shared_array<char> w_buff_data(new char[buff_size]);

  write_buff_t w_buff(stdout, w_buff_data.get(), w_buff_size);

  add_slice_t<write_buff_t&> add_slice(w_buff);
  print_out_t<write_buff_t&> print_out(w_buff);

  if (!process_stream(stdin, buff_data.get(), buff_size, add_slice, print_out)) {
    sync (w_buff);
    return 1;
  }
  
  sync (w_buff);
  return 0;
}
