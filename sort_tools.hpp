
struct inplace_str_t {
  inplace_str_t() : sz(0) {}
  inplace_str_t(size_t _sz) : sz(_sz) {}

  // header
  size_t sz;
  //char *data;

  inline char *data() { return ((char*)(void*)this) + sizeof(*this); }
  inline const char *data() const { return ((char*)(void*)this) + sizeof(*this); }
  
};


// need less operator that compares a_t with b_t
// need copy a_t and b_t to out_t withot data allocation
template <typename it_a_t, typename it_b_t, typename out_it_t>
void merge_combine(it_a_t it_a, it_a_t end_a, it_b_t it_b, it_b_t end_b, out_it_t out_it) {
  //
}

// write headers to begining and char data to the end of allocated space
// [ headers ...            ... data ]
struct slice_pool_t {
  // sz - size of headers array in bytes
  slice_pool_t(slice_t *_headers, size_t _sz)
    :headers(_headers)
    ,data_start(_sz)
    ,sz(_sz)
    ,count(0)
  {}

  // return false if no space left in pool
  bool add(const slice_t& slice) {
    // check there is enough space in pool
    if ((count + 1) * sizeof(slice_t) + slice.sz > data_start) {
      return false;
    }

    memcpy(&data[data_start - slice.sz], slice.begin, slice.sz); // copy slice to pool
    data_start -= slice.sz;

    headers[count] = slice;
    headers[count].begin = &data[data_start];

    ++count;
    return true;
  }

  slice_t* begin() {return headers;}
  slice_t* end() {return &headers[count]; }

  slice_t *headers;
  size_t sz;

  size_t count;
  size_t data_start; // start of char data
};


// merge
/*
  sorted_pool;
  new_pool; -> sort -> aggregate -> merge_with sorted_pool;

  [ sorted_headers ... sorted_data ]
  [ new_headers ]
  [ new_pool ]

  
*/ 
