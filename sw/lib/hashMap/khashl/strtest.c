// To run this program: `echo a bc a cd bc|./this_prog`
#include <stdio.h>
#include <string.h>
#include "khashl.h"
KHASHL_MAP_INIT(KH_LOCAL, strmap_t, strmap, const char*, int, kh_hash_str, kh_eq_str)

int main(int argc, char *argv[])
{
    char s[4096]; // max string length: 4095 characters
    strmap_t *h;
    khint_t k;
    h = strmap_init();
    while (scanf("%s", s) > 0) {
        int absent;
        k = strmap_put(h, s, &absent);
        if (absent) kh_key(h, k) = strdup(s), kh_val(h, k) = 0;
        // else, the key is not touched; we do nothing
        ++kh_val(h, k);
    }
    printf("# of distinct words: %d\n", kh_size(h));
    // IMPORTANT: free memory allocated by strdup() above
    kh_foreach(h, k) {
        printf("%s: %d\n", kh_key(h, k), kh_val(h, k));
        free((char*)kh_key(h, k));
    }
    strmap_destroy(h);
    return 0;
}
