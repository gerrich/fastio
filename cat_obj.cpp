#include <cstdio>
#include <inttypes.h>

#include "iotools.hpp"

template <typename file_t>
struct add_slice_t {
  add_slice_t(file_t _fd) : fd(_fd), count(0) {}

  inline void operator()(const iotools::slice_t &slice) {
    fwrite(slice.begin, 1, slice.sz, fd); 
  }

  inline size_t mem_used() const {
    return count * sizeof(iotools::slice_t);
  }

public:
  file_t fd;
  
  size_t count;
  iotools::slice_t *headers;
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



int main() {
  size_t buff_size  = 1024 * 64;
  iotools::shared_array<char> buff_data(new char[buff_size]);
  
  size_t w_buff_size  = 1024 * 32;
  iotools::shared_array<char> w_buff_data(new char[buff_size]);

  iotools::write_buff_t w_buff(stdout, w_buff_data.get(), w_buff_size);

  add_slice_t<iotools::write_buff_t&> add_slice(w_buff);
  print_out_t<iotools::write_buff_t&> print_out(w_buff);

  if (!iotools::process_stream(stdin, buff_data.get(), buff_size, add_slice, print_out)) {
    sync (w_buff);
    return 1;
  }
  
  iotools::sync(w_buff);
  return 0;
}
