/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <iostream>
#include <format>
#include <stdexcept>

#include <tclap/CmdLine.h>

int main(int argc, char** argv) {
    try {

    } catch (const std::exception& e) {
        std::println(std::cerr, "Exception: {}", e.what());
        
        return EXIT_FAILURE;
    }
}
