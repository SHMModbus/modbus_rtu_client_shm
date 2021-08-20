#pragma once

#include <modbus/modbus.h>
#include <string>

namespace Modbus {
namespace RTU {

//! Modbus RTU slave
class Slave {
private:
    modbus_t *        modbus;          //!< modbus object (see libmodbus library)
    modbus_mapping_t *mapping;         //!< modbus data object (see libmodbus library)
    bool              delete_mapping;  //!< indicates whether the mapping object was created by this instance

public:
    /*! \brief create modbus slave (TCP server)
     *
     * @param device serial device
     * @param id modbus rtu slave id
     * @param parity serial parity bit (N(one), E(ven), O(dd))
     * @param data_bits number of serial data bits
     * @param stop_bits number of serial stop bits
     * @param baud serial baud rate
     * @param rs232 connect using rs232 mode
     * @param rs485 connect using rs485 mode
     * @param mapping modbus mapping object (nullptr: an mapping object with maximum size is generated)
     */
    explicit Slave(const std::string &device,
                   int                id,
                   char               parity,
                   int                data_bits,
                   int                stop_bits,
                   int                baud,
                   bool               rs232,
                   bool               rs485,
                   modbus_mapping_t * mapping = nullptr);

    /*! \brief destroy the modbus slave
     *
     */
    ~Slave();

    /*! \brief enable/disable debugging output
     *
     * @param debug true: enable debug output
     */
    void set_debug(bool debug);

    /*! \brief wait for request from Master and generate reply
     *
     * @return true: connection closed
     */
    bool handle_request();
};

}  // namespace RTU
}  // namespace Modbus
