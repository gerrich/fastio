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

  data_t *get() { return data; }
  const data_t *get() const { return data; }
  
  data_t *data;
};

struct slice_t {
  slice_t(): sz(0) {}

  size_t sz;
  char *begin;
};

struct buff_t {
  buff_t(char *_data = NULL, size_t _cap = 0)
    : data(_data), size(0), capacity(_cap), rpos(0) {}

  char *data;
  size_t size;
  size_t capacity;
  size_t rpos;
};

bool read_line_from_buff(buff_t &buff, slice_t &slice) {
  char *it = &buff.data[buff.rpos];
  char *end = buff.data + buff.size;
  for (; it != end; ++it) {
    if (*it == '\n') { 
      slice.sz = it - &buff.data[buff.rpos] + 1;
      slice.begin = &buff.data[buff.rpos];
      buff.rpos += slice.sz;
      
      if (buff.rpos == buff.size) {
        buff.rpos = 0;
        buff.size = 0;
      }
      return true;
    }
  }
  return false;
}

const size_t IO_ERROR = -1;
// read from FD, set string begin ptr and size to slice_t
// string read - write (ptr,sz) to slice return true
// no line end found - return false  // can't process long lines
size_t read_line_to_slice(FILE* fd, buff_t &buff, slice_t &slice) {
  if (read_line_from_buff(buff, slice)) {
  printf("{%zu %zu %zu}", buff.rpos, buff.size, buff.capacity);
    return 1;
  }

  if (false && buff.rpos * 2 > buff.capacity) {
    // ignore
    return 0;
  } else {
    // move
    memmove(buff.data, &buff.data[buff.rpos], buff.size - buff.rpos);
    buff.size -= buff.rpos;
    buff.rpos = 0;
  }
  size_t read_sz = fread(buff.data, 1, buff.capacity - buff.size, fd);
  if (ferror(fd)) {
    return IO_ERROR;
  }

  buff.size += read_sz;

  if (feof(fd)) {
    slice.begin = &buff.data[buff.rpos];
    slice.sz = buff.size - buff.rpos;
  printf("{%zu %zu %zu}", buff.rpos, buff.size, buff.capacity);
    return read_sz;
  }

  size_t pfx_size = buff.size - buff.rpos;
  buff.rpos = buff.size;

  if (read_line_from_buff(buff, slice)) {
    slice.begin -= pfx_size;
    slice.sz += pfx_size;
  printf("{%zu %zu %zu}", buff.rpos, buff.size, buff.capacity);
    return read_sz;
  }

  printf("{%zu %zu %zu}", buff.rpos, buff.size, buff.capacity);
  return read_sz;
}

void copy_buff_to_fd(FILE *fd, buff_t &buff) {
  fwrite(&buff.data[buff.rpos], 1, buff.size - buff.rpos, fd);
  //buff.rpos = buff.size;
  buff.rpos = 0;
  buff.size = 0;
}


int main() {

  size_t buff_size  = 256;
  shared_array<char> buff_data(new char[buff_size]);
  buff_t buff(buff_data.get(), buff_size);

  while(!feof(stdin) || buff.rpos != buff.size) {
    slice_t slice;
    bool has_prefix = false;
    while (true) { // exit if line parsed || foef || ferror
      size_t offset = read_line_to_slice(stdin, buff, slice);
      if (slice.sz || feof(stdin)) break;
      // print line unchanged
      fwrite("[0]", 1, 3, stdout);
  printf("{%zu %zu %zu}", buff.rpos, buff.size, buff.capacity);
      copy_buff_to_fd(stdout, buff);
      has_prefix = true;
    }
    if (ferror(stdin)) {
      fwrite("[1]", 1, 3, stdout);
      copy_buff_to_fd(stdout, buff);
      return 1;
    }
    // slice - string read or suffix of previous line
    if (has_prefix) {
      fwrite("[2]", 1, 3, stdout);
      fwrite(slice.begin, 1, slice.sz, stdout);
      continue;
    }
    // new line is in slice
    //process lines
    if (slice.sz) {
      fwrite("[3]", 1, 3, stdout);
      fwrite(slice.begin, 1, slice.sz, stdout);
    } else {
      fwrite("[4]", 1, 3, stdout);
      copy_buff_to_fd(stdout, buff);
    }
  }
  printf("%zu %zu %zu\n", buff.rpos, buff.size, buff.capacity);
  return 0;
}
