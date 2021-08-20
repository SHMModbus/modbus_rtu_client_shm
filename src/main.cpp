#include <csignal>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sysexits.h>
#include <unistd.h>

#include "Modbus_RTU_Slave.hpp"
#include "modbus_shm.hpp"

//! terminate flag
static volatile bool terminate = false;

/*! \brief signal handler (SIGINT and SIGTERM)
 *
 */
static void sig_term_handler(int) {
    terminate = true;
    alarm(1);  // force termination after 1 second
}

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
        exit(EX_USAGE);
    };

    // establish signal handler
    if (signal(SIGINT, sig_term_handler) || signal(SIGTERM, sig_term_handler)) {
        perror("Failed to establish signal handler");
        exit(EX_OSERR);
    }

    if (signal(SIGALRM, [](int) { exit(EX_OK); })) {
        perror("Failed to establish signal handler");
        exit(EX_OSERR);
    }

    // all command line arguments
    options.add_options()("d,device", "mandatory: serial device", cxxopts::value<std::string>());
    options.add_options()("i,id", "mandatory: modbus RTU slave id", cxxopts::value<int>());
    options.add_options()(
            "p,parity", "serial parity bit (N(one), E(ven), O(dd))", cxxopts::value<char>()->default_value("N"));
    options.add_options()("data-bits", "serial data bits (5-8)", cxxopts::value<int>()->default_value("8"));
    options.add_options()("stop-bits", "serial stop bits (1-2)", cxxopts::value<int>()->default_value("1"));
    options.add_options()("b,baud", "serial baud", cxxopts::value<int>()->default_value("9600"));
    options.add_options()("rs485", "force to use rs485 mode");
    options.add_options()("rs232", "force to use rs232 mode");
    options.add_options()(
            "n,name-prefix", "shared memory name prefix", cxxopts::value<std::string>()->default_value("modbus_"));
    options.add_options()("do-registers",
                          "number of digital output registers",
                          cxxopts::value<std::size_t>()->default_value("65536"));
    options.add_options()(
            "di-registers", "number of digital input registers", cxxopts::value<std::size_t>()->default_value("65536"));
    options.add_options()(
            "ao-registers", "number of analog output registers", cxxopts::value<std::size_t>()->default_value("65536"));
    options.add_options()(
            "ai-registers", "number of analog input registers", cxxopts::value<std::size_t>()->default_value("65536"));
    options.add_options()("m,monitor", "output all incoming and outgoing packets to stdout");
    options.add_options()("h,help", "print usage");
    // clang-format on

    // parse arguments
    cxxopts::ParseResult args;
    try {
        args = options.parse(argc, argv);
    } catch (cxxopts::OptionParseException &e) {
        std::cerr << "Failed to parse arguments: " << e.what() << '.' << std::endl;
        exit_usage();
    }

    // print usage
    if (args.count("help")) {
        options.set_width(120);
        std::cout << options.help() << std::endl;
        std::cout << std::endl;
        std::cout << "The modbus registers are mapped to shared memory objects:" << std::endl;
        std::cout << "    type | name                      | master-access   | shm name" << std::endl;
        std::cout << "    -----|---------------------------|-----------------|----------------" << std::endl;
        std::cout << "    DO   | Discrete Output Coils     | read-write      | <name-prefix>DO" << std::endl;
        std::cout << "    DI   | Discrete Input Coils      | read-only       | <name-prefix>DI" << std::endl;
        std::cout << "    AO   | Discrete Output Registers | read-write      | <name-prefix>AO" << std::endl;
        std::cout << "    AI   | Discrete Input Registers  | read-only       | <name-prefix>AI" << std::endl;
        std::cout << std::endl;
        std::cout << "This application uses the following libraries:" << std::endl;
        std::cout << "  - cxxopts by jarro2783 (https://github.com/jarro2783/cxxopts)" << std::endl;
        std::cout << "  - libmodbus by Stéphane Raimbault (https://github.com/stephane/libmodbus)" << std::endl;
        exit(EX_OK);
    }

    // check arguments
    if (args["do-registers"].as<std::size_t>() > 0x10000) {
        std::cerr << "to many do_registers (maximum: 65536)." << std::endl;
        exit_usage();
    }

    if (args["di-registers"].as<std::size_t>() > 0x10000) {
        std::cerr << "to many do_registers (maximum: 65536)." << std::endl;
        exit_usage();
    }

    if (args["ao-registers"].as<std::size_t>() > 0x10000) {
        std::cerr << "to many do_registers (maximum: 65536)." << std::endl;
        exit_usage();
    }

    if (args["ai-registers"].as<std::size_t>() > 0x10000) {
        std::cerr << "to many do_registers (maximum: 65536)." << std::endl;
        exit_usage();
    }

    const auto PARITY = toupper(args["parity"].as<char>());
    if (PARITY != 'N' && PARITY != 'E' && PARITY != 'O') {
        std::cerr << "invalid parity" << std::endl;
        exit_usage();
    }

    const auto DATA_BITS = toupper(args["data-bits"].as<int>());
    if (DATA_BITS < 5 || DATA_BITS > 8) {
        std::cerr << "data-bits out of range" << std::endl;
        exit_usage();
    }

    const auto STOP_BITS = toupper(args["stop-bits"].as<int>());
    if (STOP_BITS < 1 || STOP_BITS > 2) {
        std::cerr << "stop-bits out of range" << std::endl;
        exit_usage();
    }

    const auto BAUD = toupper(args["baud"].as<int>());
    if (BAUD < 1) {
        std::cerr << "invalid baud rate" << std::endl;
        exit_usage();
    }

    // create shared memory object for modbus registers
    Modbus::shm::Shm_Mapping mapping(args["do-registers"].as<std::size_t>(),
                                     args["di-registers"].as<std::size_t>(),
                                     args["ao-registers"].as<std::size_t>(),
                                     args["ai-registers"].as<std::size_t>(),
                                     args["name-prefix"].as<std::string>());

    // create slave
    std::unique_ptr<Modbus::RTU::Slave> slave;
    try {
        slave = std::make_unique<Modbus::RTU::Slave>(args["device"].as<std::string>(),
                                                     args["id"].as<int>(),
                                                     PARITY,
                                                     DATA_BITS,
                                                     STOP_BITS,
                                                     BAUD,
                                                     args.count("rs232"),
                                                     args.count("rs485"),
                                                     mapping.get_mapping());
        slave->set_debug(args.count("monitor"));
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        exit(EX_SOFTWARE);
    } catch (cxxopts::option_has_no_value_exception &e) {
        std::cerr << e.what() << std::endl;
        exit_usage();
    }

    std::cerr << "Connected to bus." << std::endl;

    // ========== MAIN LOOP ========== (handle requests)
    bool connection_closed = false;
    while (!terminate && !connection_closed) {
        try {
            connection_closed = slave->handle_request();
        } catch (const std::runtime_error &e) {
            // clang-tidy (LLVM 12.0.1) warning "Condition is always true" is not correct
            if (!terminate) std::cerr << e.what() << std::endl;
            break;
        }
    }

    if (connection_closed) std::cerr << "Master closed connection." << std::endl;

    std::cerr << "Terminating..." << std::endl;
}
