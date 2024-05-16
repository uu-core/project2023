//
// Created by Caden Keese on 5/15/24.
//

#ifndef PICO_EXAMPLES_PIO_BYTECODE_GENERATOR_H
#define PICO_EXAMPLES_PIO_BYTECODE_GENERATOR_H

#include <vector>
#include <cstdint>
#include <string>

namespace pio_bytecode {

    std::vector<uint32_t> generate(const std::string &hex_str);

} // pio_bytecode

#endif //PICO_EXAMPLES_PIO_BYTECODE_GENERATOR_H
