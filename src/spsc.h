
#include <stdatomic.h>
#include <stdbool.h>

struct spsc_queue {
  _Atomic uint32_t read;
  _Atomic uint32_t write;
  const uint32_t size;
};

static inline uint32_t spsc_next_index(const struct spsc_queue *q,
                                       uint32_t idx) {
  uint32_t result = idx + 1;
  if (result >= q->size) {
    result = 0;
  }
  return result;
}

static inline bool spsc_is_empty(const struct spsc_queue *q) {
  return atomic_load_explicit(&q->read, memory_order_relaxed) ==
         atomic_load_explicit(&q->write, memory_order_relaxed);
}

static inline bool spsc_is_full(const struct spsc_queue *q) {
  return atomic_load_explicit(&q->read, memory_order_relaxed) ==
         spsc_next_index(q,
                         atomic_load_explicit(&q->write, memory_order_relaxed));
}

static inline int32_t spsc_allocate(struct spsc_queue *q) {
  uint32_t this_write = atomic_load_explicit(&q->write, memory_order_relaxed);
  uint32_t new_write = spsc_next_index(q, this_write);

  uint32_t this_read = atomic_load_explicit(&q->read, memory_order_acquire);

  if (this_read == new_write) {
    return -1;
  } else {
    return this_write;
  }
}

static inline void spsc_push(struct spsc_queue *q) {
  uint32_t this_write = atomic_load_explicit(&q->write, memory_order_relaxed);
  uint32_t new_write = spsc_next_index(q, this_write);

  atomic_store_explicit(&q->write, new_write, memory_order_release);
}

static inline uint32_t spsc_next(struct spsc_queue *q) {
  uint32_t this_read = atomic_load_explicit(&q->read, memory_order_relaxed);
  uint32_t this_write = atomic_load_explicit(&q->write, memory_order_acquire);

  if (this_read == this_write) {
    return -1;
  } else {
    return this_read;
  }
}

static inline void spsc_release(struct spsc_queue *q) {
  uint32_t this_read = atomic_load_explicit(&q->read, memory_order_relaxed);
  atomic_store_explicit(
    &q->read, spsc_next_index(q, this_read), memory_order_release);
}
