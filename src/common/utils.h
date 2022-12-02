/**
 * @file utils.h
 * @author  Federico Roux (federico.roux@globant.com)
 * @brief  utils.cpp header file
 * @version 0.1
 * @date 2022-11-25
 */
#ifndef __UTILS_H
#define __UTILS_H

#include <string>
#include <stdint.h>

namespace utils {

    static constexpr uint16_t port_range_min = 0;
    static constexpr uint16_t port_range_max = 65535;

    bool validate_ip(std::string ip);
    bool validate_port(uint16_t p);

};

#endif // __UTILS_H