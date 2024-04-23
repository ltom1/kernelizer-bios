#include <types.h>


// writes <n_bytes> times <val> to <dest>
void mem_set(u8 *dest, u8 val, u64 n_bytes) {

    for (u64 i = 0; i < n_bytes; i++) {
        *(dest + i) = val;
    }
}


// copies <n_bytes> from <src> to <dest>
void mem_cpy(u8 *dest, u8 *src, u64 n_bytes) {
 
    for (u64 i = 0; i < n_bytes; i++) {
          *(dest + i) = *(src + i);
     }
}
