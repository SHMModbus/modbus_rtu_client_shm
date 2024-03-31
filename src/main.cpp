/*
 * Copyright (C) 2021-2022 Nikolas Koesling <nikolas@koesling.info>.
 * This program is free software. You can redistribute it and/or modify it under the terms of the GPLv3 License.
 */

#include "Modbus_RTU_Client.hpp"
#include "Print_Time.hpp"
#include "generated/version_info.hpp"
#include "license.hpp"
#include "modbus_shm.hpp"

#include <csignal>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/signalfd.h>
#include <sysexits.h>
#include <unistd.h>

//! Help output line width
static constexpr std::size_t HELP_WIDTH = 120;

//! terminate flag
static volatile bool terminate = false;

//! modbus socket (to be closed if termination is requested)
static int socket = -1;

/*! \brief signal handler (SIGINT and SIGTERM)
 *
 */
static void sig_term_handler(int) {
    if (socket != -1) close(socket);
    terminate = true;
}

constexpr std::array<int, 10> TERM_SIGNALS = {SIGINT,
                                              SIGTERM,
                                              SIGHUP,
                                              SIGIO,  // should not happen
                                              SIGPIPE,
                                              SIGPOLL,  // should not happen
                                              SIGPROF,  // should not happen
                                              SIGUSR1,
                                              SIGUSR2,
                                              SIGVTALRM};

/*! \brief main function
 *
 * @param argc number of arguments
 * @param argv arguments as char* array
 * @return exit code
 */
int main(int argc, char **argv) {
    const std::string exe_name = std::filesystem::path(argv[0]).filename().string();
    cxxopts::Options  options(exe_name, "Modbus client that uses shared memory objects to store its register values");

    auto exit_usage = [&exe_name]() {
        std::cerr << "Use '" << exe_name << " --help' for more information." << std::endl;
        return EX_USAGE;
    };

    auto euid = geteuid();
    if (!euid) std::cerr << "!!!! WARNING: You should not execute this program with root privileges !!!!" << std::endl;

#ifdef COMPILER_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    // establish signal handler
    struct sigaction term_sa;
    term_sa.sa_handler = sig_term_handler;
    term_sa.sa_flags   = SA_RESTART;
    sigemptyset(&term_sa.sa_mask);
    for (const auto SIGNO : TERM_SIGNALS) {
        if (sigaction(SIGNO, &term_sa, nullptr)) {
            perror("Failed to establish signal handler");
            return EX_OSERR;
        }
    }
#ifdef COMPILER_CLANG
#    pragma clang diagnostic pop
#endif

    // all command line arguments
    options.add_options("serial")("d,device", "mandatory: serial device", cxxopts::value<std::string>());
    options.add_options("serial")("i,id", "mandatory: modbus RTU client id", cxxopts::value<int>());
    options.add_options("serial")(
            "p,parity", "serial parity bit (N(one), E(ven), O(dd))", cxxopts::value<char>()->default_value("N"));
    options.add_options("serial")("data-bits", "serial data bits (5-8)", cxxopts::value<int>()->default_value("8"));
    options.add_options("serial")("stop-bits", "serial stop bits (1-2)", cxxopts::value<int>()->default_value("1"));
    options.add_options("serial")("b,baud", "serial baud", cxxopts::value<int>()->default_value("9600"));
    options.add_options("serial")("rs485", "force to use rs485 mode");
    options.add_options("serial")("rs232", "force to use rs232 mode");
    options.add_options("shared memory")(
            "n,name-prefix", "shared memory name prefix", cxxopts::value<std::string>()->default_value("modbus_"));
    options.add_options("modbus")("do-registers",
                                  "number of digital output registers",
                                  cxxopts::value<std::size_t>()->default_value("65536"));
    options.add_options("modbus")(
            "di-registers", "number of digital input registers", cxxopts::value<std::size_t>()->default_value("65536"));
    options.add_options("modbus")(
            "ao-registers", "number of analog output registers", cxxopts::value<std::size_t>()->default_value("65536"));
    options.add_options("modbus")(
            "ai-registers", "number of analog input registers", cxxopts::value<std::size_t>()->default_value("65536"));
    options.add_options("modbus")("m,monitor", "output all incoming and outgoing packets to stdout");
    options.add_options("modbus")("byte-timeout",
                                  "timeout interval in seconds between two consecutive bytes of the same message. "
                                  "In most cases it is sufficient to set the response timeout. "
                                  "Fractional values are possible.",
                                  cxxopts::value<double>());
    options.add_options("modbus")(
            "response-timeout",
            "set the timeout interval in seconds used to wait for a response. "
            "When a byte timeout is set, if the elapsed time for the first byte of response is longer "
            "than the given timeout, a timeout is detected. "
            "When byte timeout is disabled, the full confirmation response must be received before "
            "expiration of the response timeout. "
            "Fractional values are possible.",
            cxxopts::value<double>());
    options.add_options("shared memory")(
            "force",
            "Force the use of the shared memory even if it already exists. "
            "Do not use this option per default! "
            "It should only be used if the shared memory of an improperly terminated instance continues "
            "to exist as an orphan and is no longer used.");
    options.add_options("shared memory")("semaphore",
                                         "protect the shared memory with a named semaphore against simultaneous access",
                                         cxxopts::value<std::string>());
    options.add_options("shared memory")(
            "semaphore-force",
            "Force the use of the semaphore even if it already exists. "
            "Do not use this option per default! "
            "It should only be used if the semaphore of an improperly terminated instance continues "
            "to exist as an orphan and is no longer used.");
    options.add_options("shared memory")("permissions",
                                         "permission bits that are applied when creating a shared memory.",
                                         cxxopts::value<std::string>()->default_value("0640"));
    options.add_options("other")("h,help", "print usage");
    options.add_options("version information")("version", "print version and exit");
    options.add_options("version information")("longversion",
                                               "print version (including compiler and system info) and exit");
    options.add_options("version information")("shortversion", "print version (only version string) and exit");
    options.add_options("version information")("git-hash", "print git hash");
    options.add_options("other")("license", "show licences (short)");
    options.add_options("other")("license-full", "show licences (full license text)");

    // parse arguments
    cxxopts::ParseResult args;
    try {
        args = options.parse(argc, argv);
    } catch (cxxopts::exceptions::parsing::exception &e) {
        std::cerr << Print_Time::iso << " ERROR: Failed to parse arguments: " << e.what() << ".'\n";
        return exit_usage();
    }

    // print usage
    if (args.count("help")) {
        options.set_width(HELP_WIDTH);
#ifdef OS_LINUX
        if (isatty(STDIN_FILENO)) {
            struct winsize w {};
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1) {  // NOLINT
                static constexpr auto MIN_TTY_SIZE = static_cast<decltype(w.ws_col)>(80);
                options.set_width(std::max(MIN_TTY_SIZE, w.ws_col));
            }
        }
#endif

        std::cout << options.help() << '\n';
        std::cout << '\n';
        std::cout << "The modbus registers are mapped to shared memory objects:" << '\n';
        std::cout << "    type | name                      | mb-server-access | shm name" << '\n';
        std::cout << "    -----|---------------------------|------------------|----------------" << '\n';
        std::cout << "    DO   | Discrete Output Coils     | read-write       | <name-prefix>DO" << '\n';
        std::cout << "    DI   | Discrete Input Coils      | read-only        | <name-prefix>DI" << '\n';
        std::cout << "    AO   | Discrete Output Registers | read-write       | <name-prefix>AO" << '\n';
        std::cout << "    AI   | Discrete Input Registers  | read-only        | <name-prefix>AI" << '\n';
        std::cout << '\n';
        std::cout << "This application uses the following libraries:" << '\n';
        std::cout << "  - cxxopts by jarro2783 (https://github.com/jarro2783/cxxopts)" << '\n';
        std::cout << "  - libmodbus by Stéphane Raimbault (https://github.com/stephane/libmodbus)" << '\n';
        std::cout << "  - cxxshm (https://github.com/NikolasK-source/cxxshm)" << '\n';
        std::cout << "  - cxxsemaphore (https://github.com/NikolasK-source/cxxsemaphore)" << '\n';
        return EX_OK;
    }

    // print version
    if (args.count("longversion")) {
        std::cout << PROJECT_NAME << ' ' << PROJECT_VERSION << " (compiled with " << COMPILER_INFO << " on "
                  << SYSTEM_INFO << ')'
#ifndef OS_LINUX
                  << "-nonlinux"
#endif
                  << '\n';
        return EX_OK;
    }

    if (args.count("shortversion")) {
        std::cout << PROJECT_VERSION << '\n';
        return EX_OK;
    }

    if (args.count("version")) {
        std::cout << PROJECT_NAME << ' ' << PROJECT_VERSION << '\n';
        return EX_OK;
    }

    if (args.count("longversion")) {
        std::cout << PROJECT_NAME << ' ' << PROJECT_VERSION << '\n';
        std::cout << "   compiled with " << COMPILER_INFO << '\n';
        std::cout << "   on system " << SYSTEM_INFO
#ifndef OS_LINUX
                  << "-nonlinux"
#endif
                  << '\n';
        std::cout << "   from git commit " << RCS_HASH << '\n';
        return EX_OK;
    }

    if (args.count("git-hash")) {
        std::cout << RCS_HASH << '\n';
        return EX_OK;
    }

    // print licenses
    if (args.count("license")) {
        print_licenses(std::cout, false);
        return EX_OK;
    }

    if (args.count("license-full")) {
        print_licenses(std::cout, false);
        return EX_OK;
    }

    // check arguments
    if (args["do-registers"].as<std::size_t>() > 0x10000) {
        std::cerr << "to many do_registers (maximum: 65536)." << std::endl;
        return exit_usage();
    }

    if (args["di-registers"].as<std::size_t>() > 0x10000) {
        std::cerr << "to many do_registers (maximum: 65536)." << std::endl;
        return exit_usage();
    }

    if (args["ao-registers"].as<std::size_t>() > 0x10000) {
        std::cerr << "to many do_registers (maximum: 65536)." << std::endl;
        return exit_usage();
    }

    if (args["ai-registers"].as<std::size_t>() > 0x10000) {
        std::cerr << "to many do_registers (maximum: 65536)." << std::endl;
        return exit_usage();
    }

    const auto PARITY = toupper(args["parity"].as<char>());
    if (PARITY != 'N' && PARITY != 'E' && PARITY != 'O') {
        std::cerr << "invalid parity" << std::endl;
        return exit_usage();
    }

    const auto DATA_BITS = toupper(args["data-bits"].as<int>());
    if (DATA_BITS < 5 || DATA_BITS > 8) {
        std::cerr << "data-bits out of range" << std::endl;
        return exit_usage();
    }

    const auto STOP_BITS = toupper(args["stop-bits"].as<int>());
    if (STOP_BITS < 1 || STOP_BITS > 2) {
        std::cerr << "stop-bits out of range" << std::endl;
        return exit_usage();
    }

    const auto BAUD = toupper(args["baud"].as<int>());
    if (BAUD < 1) {
        std::cerr << "invalid baud rate" << std::endl;
        return exit_usage();
    }

    if (args["rs232"].count() && args["rs485"].count()) {
        std::cerr << "Cannot operate in RS232 and RS485 mode at the same time." << std::endl;
        return exit_usage();
    }

    // SHM permissions
    mode_t shm_permissions = 0660;
    {
        const auto  shm_permissions_str = args["permissions"].as<std::string>();
        bool        fail                = false;
        std::size_t idx                 = 0;
        try {
            shm_permissions = std::stoul(shm_permissions_str, &idx, 0);
        } catch (const std::exception &) { fail = true; }
        fail = fail || idx != shm_permissions_str.size();

        if (fail || (~static_cast<mode_t>(0x1FF) & shm_permissions) != 0) {
            std::cerr << Print_Time::iso << " ERROR: Invalid file permissions \"" << shm_permissions_str << '"'
                      << std::endl;
            return EX_USAGE;
        }
    }

    // create shared memory object for modbus registers
    std::unique_ptr<Modbus::shm::Shm_Mapping> mapping;
    try {
        mapping = std::make_unique<Modbus::shm::Shm_Mapping>(args["do-registers"].as<std::size_t>(),
                                                             args["di-registers"].as<std::size_t>(),
                                                             args["ao-registers"].as<std::size_t>(),
                                                             args["ai-registers"].as<std::size_t>(),
                                                             args["name-prefix"].as<std::string>(),
                                                             args.count("force") > 0,
                                                             shm_permissions);
    } catch (const std::system_error &e) {
        std::cerr << e.what() << std::endl;
        return EX_OSERR;
    }

    // create client
    std::unique_ptr<Modbus::RTU::Client> client;
    try {
        client = std::make_unique<Modbus::RTU::Client>(args["device"].as<std::string>(),
                                                       args["id"].as<int>(),
                                                       PARITY,
                                                       DATA_BITS,
                                                       STOP_BITS,
                                                       BAUD,
                                                       args.count("rs232"),
                                                       args.count("rs485"),
                                                       mapping->get_mapping());
        client->set_debug(args.count("monitor"));
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        return EX_SOFTWARE;
    } catch (cxxopts::exceptions::option_has_no_value::exception &e) {
        std::cerr << e.what() << std::endl;
        return exit_usage();
    }
    socket = client->get_socket();

    // set timeouts if required
    try {
        if (args.count("response-timeout")) { client->set_response_timeout(args["response-timeout"].as<double>()); }

        if (args.count("byte-timeout")) { client->set_byte_timeout(args["byte-timeout"].as<double>()); }
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        return EX_SOFTWARE;
    }

    // add semaphore if required
    try {
        if (args.count("semaphore")) {
            client->enable_semaphore(args["semaphore"].as<std::string>(), args.count("semaphore-force"));
        }
    } catch (const std::system_error &e) {
        std::cerr << Print_Time::iso << " ERROR: " << e.what() << std::endl;
        return EX_SOFTWARE;
    }

    std::cerr << Print_Time::iso << " INFO: Connected to bus." << std::endl;

    // ========== MAIN LOOP ========== (handle requests)
    bool connection_closed = false;
    while (!terminate && !connection_closed) {
        try {
            connection_closed = client->handle_request();
        } catch (const std::runtime_error &e) {
            // clang-tidy (LLVM 12.0.1) warning "Condition is always true" is not correct
            if (!terminate) std::cerr << e.what() << std::endl;
            break;
        }
    }

    if (connection_closed) std::cerr << Print_Time::iso << " INFO: Modbus Server closed connection." << std::endl;

    std::cerr << "Terminating..." << std::endl;
}
