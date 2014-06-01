#include <cstdio>
#include <inttypes.h>

#include "iotools.hpp"

namespace iotools {

template <typename file_t>
struct add_slice_t {
  add_slice_t(file_t _fd) : fd(_fd), count(0) {}

  inline void operator()(const slice_t &slice) {
    //fwrite(slice.begin, 1, slice.sz, fd); 
    
    headers[count] = slice;
    ++count;
  }

  inline size_t mem_used() const {
    return count * sizeof(slice_t);
  }

  // call this before data in buffer become invalid
  inline void commit() {
    for (size_t i = 0; i < count; ++i) {
      const slice_t &slice = headers[i];
      fwrite(slice.begin, 1, slice.sz, fd);
    }
    count = 0;
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

} // namespace iotools {

using namespace iotools;

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
  
  sync(w_buff);
  return 0;
}
