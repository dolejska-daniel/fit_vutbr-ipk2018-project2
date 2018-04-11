// general.h
// IPK-PROJ2, 07.04.2018
// Author: Daniel Dolejska, FIT

#ifndef IPK_DHCPSTARVE_GENERAL_H
#define IPK_DHCPSTARVE_GENERAL_H

#include <stdint.h>

/**
 * Vygeneruje nahodnou MAC adresu.
 *
 * @return
 */
uint8_t *create_mac();

/**
 * Uvolni alokovanou pamet pro MAC adresu.
 *
 * @param mac
 */
void destroy_mac(uint8_t *mac);

/**
 * Vygeneruje nahodne cislo od nuly do hodnoty parametru max.
 *
 * @param max
 *
 * @return
 */
uint64_t create_random_number( uint64_t max );

#endif //IPK_DHCPSTARVE_GENERAL_H
