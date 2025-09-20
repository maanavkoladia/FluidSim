#include <stdio.h>
#include <stdint.h>
#include "khashl.h"
// Instantiate
KHASHL_MAP_INIT(KH_LOCAL, map32_t, map32, uint32_t, int, kh_hash_uint32, kh_eq_generic)

int main(void) {
    int absent;
    khint_t k;
    map32_t *h = map32_init();
    k = map32_put(h, 20, &absent); // get iterator to the new bucket
    kh_val(h, k) = 2; // set value
    k = map32_get(h, 30); // query the hash table
    if (k < kh_end(h)) printf("found key '30'\n");
    kh_foreach(h, k) { // iterate
        printf("h[%u]=%d\n", kh_key(h, k), kh_val(h, k));
    }
    map32_destroy(h); // free
    return 0;
}
