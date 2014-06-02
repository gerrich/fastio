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
  typedef data_t data_type;
  typedef std::list<rope_segment_t> storage_t;
  
  struct iterator {
    iterator(size_t _offset, storage_t::iterator _it)
      : offset(_offset)
      , it(_it) 
    {}
    size_t offset;
    storage_t::iterator it;  
  };

  rope_t(): seg_size(1024) {}

  iterator begin() {
    iterator it(0, storage.begin());
    if (it.it != storage.end()) {
      it.offset = it.it->begin;
    }
    return it;
  }

  iterator end() {
    return iterator(0, storage.end());
  }
  
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


template <typename data_t>
struct no_aggregate_t {
  void operator()(data_t &dst, data_t &src) const {
    // do nothing
  }
};

template <typename rope_t, typename compare_t, typename aggregate_t /*= no_aggregate_t<class rope_t::data_type>*/ >
void merge_aggregate(
  rope_t &rope_l,
  rope_t &rope_r,
  rope_t res,
  const compare_t &compare=compare_t(),
  const aggregate_t &aggregate=aggregate_t()
) {

  while (!rope_l.empty() or !rope_r.empty()) {
    typename rope_t::data_type &data_l = rope_l.front();
    typename rope_t::data_type &data_r = rope_r.front();

    int _cmp = compare(data_l, data_r);
    if (_cmp < 0) {
      res.push_back(data_l);
      rope_l.pop_front();
    } else if (_cmp > 0) {
      res.push_back(data_r);
      rope_r.pop_front();
    } else {
      res.push_back(data_l);
      aggregate(res.back(), data_r);
      rope_l.pop_front();
      rope_r.pop_front();
    }
  }
  while(!rope_l.empty()) {
    res.push_back(rope_l.front());
    rope_l.pop_front();
  }
  while(!rope_r.empty()) {
    res.push_back(rope_r.front());
    rope_r.pop_front();
  }
}

template <typename rope_t, typename compare_t, typename aggregate_t /*= no_aggregate_t<class rope_t::data_type>*/ >
void sort_aggregate(
  rope_t &rope,
  const compare_t &compare,
  const aggregate_t &aggregate
) {
  rope_t tmp;
  
  for (size_t chunk_size = 1;; chunk_size *= 2) {
    typename rope_t::iterator it_l = rope.begin();
    typename rope_t::iterator it_r = rope.begin();
    for (size_t i = 0; i < chunk_size; ++i) {
      ++it_r;
    }
    typename rope_t::iterator end_l = it_r;
    
    char* anchor = push_marker(tmp);
    merge_aggregate(it_l, it_r, chunk_size, tmp);

  }
}
