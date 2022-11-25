/**
 * @file utils.cpp
 * @author  Federico Roux (federico.roux@globant.com)
 * @brief   Common functions
 * @version 0.1
 * @date 2022-11-23
 * 
 */

#include <string>
#include <stdint.h>
#include <arpa/inet.h>

namespace utils {

    static constexpr uint16_t port_range_min = 0;
    static constexpr uint16_t port_range_max = 65535;

    /**
     * @brief validate ip ipv4 format
     * 
     * @param ip 
     * @return true valid IP
     * @return false unvalid IP
     * 
     * @author federico.roux@globant.com
     */
    bool validate_ip(std::string ip) {
    struct sockaddr_in sa;
    auto res = inet_pton(AF_INET, ip.c_str(),  &(sa.sin_addr));
    return res != 0;
    }

    /**
     * @brief Validate IP port range
     * 
     * @param p 
     * @return true port in range
     * @return false port out of range
     */

    bool validate_port(uint16_t p) {
        return ((p > port_range_min) && (p < port_range_max));
    }

};
