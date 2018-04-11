// general.c
// IPK-PROJ2, 07.04.2018
// Author: Daniel Dolejska, FIT


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>

#include "general.h"

uint64_t create_random_number( uint64_t max )
{
    return rand() % max;
}

uint8_t *create_mac()
{
    uint8_t i, tp;
    uint8_t *result = malloc(sizeof(uint8_t) * 6);

    for (i = 0; i < 6; i++)
    {
        tp = (uint8_t) create_random_number(256);
        if (i == 0)
            tp &= ~((uint8_t)3);

        result[i] = tp;
    }

    return result;
}

void destroy_mac( uint8_t *mac )
{
    free(mac);
}