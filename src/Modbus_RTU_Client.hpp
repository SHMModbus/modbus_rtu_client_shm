/*
 * Copyright (C) 2021-2022 Nikolas Koesling <nikolas@koesling.info>.
 * This program is free software. You can redistribute it and/or modify it under the terms of the GPLv3 License.
 */

#pragma once

#include <modbus/modbus.h>
#include <string>

namespace Modbus {
namespace RTU {

//! Modbus RTU client
class Client {
private:
    modbus_t         *modbus;          //!< modbus object (see libmodbus library)
    modbus_mapping_t *mapping;         //!< modbus data object (see libmodbus library)
    bool              delete_mapping;  //!< indicates whether the mapping object was created by this instance
    int               socket = -1;     //!< internal modbus communication socket

public:
    /*! \brief create modbus client (TCP server)
     *
     * @param device serial device
     * @param id modbus rtu client id
     * @param parity serial parity bit (N(one), E(ven), O(dd))
     * @param data_bits number of serial data bits
     * @param stop_bits number of serial stop bits
     * @param baud serial baud rate
     * @param rs232 connect using rs232 mode
     * @param rs485 connect using rs485 mode
     * @param mapping modbus mapping object (nullptr: an mapping object with maximum size is generated)
     */
    explicit Client(const std::string &device,
                    int                id,
                    char               parity,
                    int                data_bits,
                    int                stop_bits,
                    int                baud,
                    bool               rs232,
                    bool               rs485,
                    modbus_mapping_t  *mapping = nullptr);

    /*! \brief destroy the modbus client
     *
     */
    ~Client();

    /*! \brief enable/disable debugging output
     *
     * @param debug true: enable debug output
     */
    void set_debug(bool debug);

    /*! \brief wait for request from Modbus Server and generate reply
     *
     * @return true: connection closed
     */
    bool handle_request();

    /*!
     * \brief set byte timeout
     *
     * @details see https://libmodbus.org/docs/v3.1.7/modbus_set_byte_timeout.html
     *
     * @param timeout byte timeout in seconds
     */
    void set_byte_timeout(double timeout);

    /*!
     * \brief set byte timeout
     *
     * @details see https://libmodbus.org/docs/v3.1.7/modbus_set_response_timeout.html
     *
     * @param timeout byte response in seconds
     */
    void set_response_timeout(double timeout);

    /**
     * \brief get byte timeout in seconds
     * @return byte timeout
     */
    double get_byte_timeout();

    /**
     * \brief get response timeout in seconds
     * @return response timeout
     */
    double get_response_timeout();

    /*! \brief get the modbus socket
     *
     * @return socket of the modbus connection
     */
    [[nodiscard]] int get_socket() const noexcept { return socket; }
};

}  // namespace RTU
}  // namespace Modbus
