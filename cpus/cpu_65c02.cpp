#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <mach/mach_time.h>
#include <getopt.h>

#include "gs2.hpp"
#include "cpu.hpp"
#include "clock.hpp"
#include "memory.hpp"
#include "opcodes.hpp"
#include "debug.hpp"

#define CPU_65C02

// 65c02-specific wrapper

namespace cpu_65c02 {

#include "core_6502.hpp"

#include "core_6502.cpp"

}
