#include <types.h>
#include <heap.h>
#include <log.h>
#include <tty.h>
#include <x86.h>


// allocate <n_bytes> on the heap (bump/stack allocator)
void *heap_alloc(heap_t *self, u64 n_bytes) {

    void *res = self->loc + self->top;
    self->top += n_bytes;
    self->free -= n_bytes;

    if (self->top > self->size) log_err("Allocation exceeds heap size\n");

    return res;
}
