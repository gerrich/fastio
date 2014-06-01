#include <list>
#include <cstring>

struct rope_segment_t {
  rope_segment_t(char *_data=NULL, size_t _sz = 0)
    :data(_data)
    ,sz(_sz)
  {}

  char *data;
  size_t sz;
  size_t begin;
  size_t end;
};



// a) trivial for fixed length types
// b) zero terminates trings
// c) headers based variable length data
// d) two side filled types

// allocate/free memory using new/delete
// take segments from pool or allocate new
// 
template <typename data_t, typename fill_strategy_t, typename mem_use_strategy_t>
struct rope_t {
  rope_t(): seg_size(1024) {}

  void push_back(const data_t &data); // add segments as memory needed
  void pop_front();
  data_t& front();
  const data_t& front() const;
  data_t& back();
  const data_t& back() const;

  bool empty() const {
    if ( storage.empty()) {
      return true;
    } else {
      // TODO enshure no empty segments left
      for (storage_t::const_iterator it = storage.begin(); it != storage.end(); ++it) {
        if (it->begin != it->end) return false;
      }
      return true;
    }
  }

  size_t mem_left_back() const {
    if (storage.empty()) {
      return 0;
    }
    const rope_segment_t& seg = storage.back();
    return seg.sz - seg.end;
  }

  typedef std::list<rope_segment_t> storage_t;
  storage_t storage;
  size_t seg_size;
};

template <typename data_t, typename fill_strategy_t, typename mem_use_strategy_t>
void rope_t<data_t, fill_strategy_t, mem_use_strategy_t>::push_back(const data_t &data) {
  size_t mem_need = sizeof(data);
  size_t mem_left = mem_left_back(); 

  if (mem_left < mem_need) {
    char *seg_data = (char*)new data_t[seg_size]; // or pop from segment pool
    rope_segment_t seg(seg_data, seg_size * sizeof(data_t));
    storage.push_back(seg);
  }

  rope_segment_t &seg = storage.back();
  memcpy(&seg.data[seg.end], (char*)&data, sizeof(data)); // change to data_t.operator=
  seg.end += sizeof(data);
}

template <typename data_t, typename fill_strategy_t, typename mem_use_strategy_t>
data_t& rope_t<data_t, fill_strategy_t, mem_use_strategy_t>::front() {
  rope_segment_t &seg = storage.front();
  return *((data_t*)&seg.data[seg.begin]);
}

template <typename data_t, typename fill_strategy_t, typename mem_use_strategy_t>
void rope_t<data_t, fill_strategy_t, mem_use_strategy_t>::pop_front() {
  rope_segment_t &seg = storage.front();
  // destructor *((data_t*)&seg.data[seg.begin])
  seg.begin += sizeof(data_t);

  if (seg.begin == seg.end) {
    delete [] seg.data; // or pop data to segment_pool
    storage.pop_front();
  }
}

template <typename data_t, typename fill_strategy_t, typename mem_use_strategy_t>
data_t& rope_t<data_t, fill_strategy_t, mem_use_strategy_t>::back() {
  rope_segment_t &seg = storage.back();
  return *((data_t*)&seg.data[seg.end - sizeof(data_t)]);
}


