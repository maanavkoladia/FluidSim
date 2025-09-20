#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
    uint32_t ip;
    uint8_t macAddress[6];
}netinfo_t;

#define NAME net_map
#define KEY_TY uint32_t
#define VAL_TY char*
#define MAX_LOAD .8

#include "verstable.h"

void PrintNetinfo(netinfo_t* buf) {
    // Print IP address in dotted decimal format
    // Accessing individual bytes of the uint32_t for printing
    printf("IP Address: %u.%u.%u.%u\n",
           (buf->ip >> 24) & 0xFF,
           (buf->ip >> 16) & 0xFF,
           (buf->ip >> 8) & 0xFF,
           buf->ip & 0xFF);

    // Print MAC address in hexadecimal format separated by colons
    printf("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
           buf->macAddress[0],
           buf->macAddress[1],
           buf->macAddress[2],
           buf->macAddress[3],
           buf->macAddress[4],
           buf->macAddress[5]);
}

int main(void){

    printf("Test starting up\n");

    net_map mymap;
    net_map_itr itr;
    netinfo_t list[10];

    net_map_init(&mymap);

    printf("Inserting\n");

    for(int i = 0; i < 10; i++){
        list[i].ip = i;
        uint8_t mac[6] = {i+1,i+2,i+3,i+4,i+5,0};
        memcpy(list[i].macAddress, mac , 6);
    }

    //for(int i = 0; i < 10; i++){
    //    PrintNetinfo(&list[i]);
    //}

    for (int i = 0; i < 10; i++){
        netinfo_t* curr = &list[i];

        net_map_itr itr = net_map_insert(&mymap, curr->ip, (char*)curr->macAddress);

        if(vt_is_end(itr)){
            vt_cleanup(&mymap);
            printf("Got error \n");
        }else{
            printf("Inserted: \n");
            PrintNetinfo(curr);
        }
    }

    printf("printing\n");
    int i = 0; 
    for(net_map_itr itr = net_map_first(&mymap); !(net_map_is_end(itr)); net_map_next(itr)){
        if(i++ >= 10 ){
            break;  
        }
        //PrintNetinfo(itr.data->val);
        printf("%s\n", itr.data->val);
    }

    vt_cleanup(&mymap);

    return EXIT_SUCCESS;
}




