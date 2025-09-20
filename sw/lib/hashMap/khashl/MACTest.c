#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "khashl.h"

#define UNIQUE_MAC_ADRESS_BYTES (3)

typedef union{
    uint32_t macAddress_24b;
    uint8_t macAddress_3B[UNIQUE_MAC_ADRESS_BYTES];
}macAddress_t;
// Correct signatures for khashl
void print_mac(const macAddress_t* mac) {
    printf("%02X.%02X.%02X\n", mac->macAddress_3B[0], mac->macAddress_3B[1], mac->macAddress_3B[2]);
}

void HT_Put(macAddress_t* mac){
    
}

macAddress_t exmac = {
    .macAddress_3B = {0x01, 0x02, 0x03}
};
int absent;

int main(void) {
    macmap_t *map = macmap_init();
    if (!map) {
        fprintf(stderr, "Failed to init map\n");
        return 1;
    }

    macAddress_t* macBuf = malloc(sizeof(macAddress_t));
    memcpy(macBuf, &exmac, sizeof(macAddress_t));

    khint_t k = macmap_put(map, macBuf, &absent);

    if(absent){
        kh_val(map, k) = 2;
    }else{
        kh_val(map, k)++;
        free(macBuf);
    }

    // Print and clean up
    k = macmap_get(map, &exmac);
    if (k != kh_end(map)) { // correct way to check if found
        print_mac(kh_key(map, k));
        printf("Val: %d\n", kh_val(map, k));
        free(kh_key(map, k)->macAddress_3B);
    } else {
        printf("Mac not found\n");
    }
    
    macmap_destroy(map);
    return 0;
}


// Sample MACs

//macAddress_t macs[] = {
//    {{.macAddress_3B = {0x01, 0x02, 0x03}}};
//    {{.macAddress_3B = {0x01, 0x02, 0x03}}};
//    {{.macAddress_3B = {0xAA, 0xBB, 0xCC}}};
//    {{.macAddress_3B = {0x01, 0x02, 0x03}}};
//    {{.macAddress_3B = {0xAA, 0xBB, 0xCC}}}
//};
