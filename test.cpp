
#include <functional>
#include <cassert>
#include <iostream>
#include <unistd.h>

#include "gs2.hpp"
#include "cpu.hpp"
#include "opcodes.hpp"
#include "test.hpp"
#include "memory.hpp"

void run_test(Test& test) {
    cpu_state *cpu = &CPUs[0];
    uint32_t addr = test.program_address;

    // Load the program into memory
    for (uint32_t i = 0; i < test.program_size; i++) {
        raw_memory_write(cpu, addr++, test.program[i]);
    }

    // Set up vectors
    raw_memory_write_word(cpu, RESET_VECTOR, test.program_address);
    raw_memory_write_word(cpu, BRK_VECTOR, test.program_address);

    // Run setup function
    if (test.setup) {
        test.setup(cpu);
    }

    // Run the CPU
    run_cpus();

    // Run assertions
    if (test.assertions) {
        test.assertions(cpu);
    }
}

void assert_cycles(cpu_state *cpu, uint64_t expected_cycles) {
    if (cpu->cycles == expected_cycles) return;
    std::cout << expected_cycles << " expected, got " << cpu->cycles << std::endl;
    assert(cpu->cycles == expected_cycles);
}

void demo_ram() {
    Test test = {
        .program = {
            OP_LDX_IMM, 0xD0,       // 2 cycles
            OP_LDA_ZP_X, 0x00,      // 3 cycles
            OP_LDA_ZP_X, 0x40,      // 3 cycles

            OP_LDA_ABS, 0x12, 0x34, // 4 cycles
            OP_LDA_ABS_X, 0x12, 0x34, // 4 cycles?

            OP_LDY_IMM, 0x67,           
            OP_LDX_ABS_Y, 0x12, 0x34, // 4 cycles
            OP_LDX_IMM, 0x56, // 2 cycles
            OP_LDY_ABS_X, 0x12, 0x34, // 
            
            OP_LDA_IND_X, 0x40, // 5 cycles
            OP_LDY_IMM, 0x89, // 2 cycles
            OP_LDA_IND_Y, 0x40, // 5 cycles

            OP_LDX_ZP_Y, 0x50, // 3 cycles
            OP_LDY_ZP_X, 0x50, // 3 cycles
           
            OP_LDA_IMM, 0xAA,
            OP_ORA_IMM, 0x55,
            OP_STA_ZP, 0xFF, 

            OP_LDA_IMM, 0xAA,
            OP_EOR_IMM, 0xFF,
            OP_STA_ZP, 0x60,

            OP_DEX_IMP,
            OP_DEY_IMP,

    // overflow 0x50 + 0x10 = 0x60, v = 0

            OP_CLC_IMP,         
            OP_LDA_IMM, 0x50,
            OP_ADC_IMM, 0x10,

    // overflow 0x50 + 0x50 = 0xA0, v = 1 
            OP_CLC_IMP,         
            OP_LDA_IMM, 0x50,
            OP_ADC_IMM, 0x50,

    // overflow 0x50 + 0x90 = 0xE0, v = 0 
            OP_CLC_IMP,         
            OP_LDA_IMM, 0x50,
            OP_ADC_IMM, 0x90,

    // overflow 0x50 + 0xD0 = 0x120, v = 0 
            OP_CLC_IMP,         
            OP_LDA_IMM, 0x50,
            OP_ADC_IMM, 0xD0,

    // overflow 0xD0 + 0x10 = 0xE0, v = 0 
            OP_CLC_IMP,         
            OP_LDA_IMM, 0xD0,
            OP_ADC_IMM, 0x10,

    // overflow 0xD0 + 0x50 = 0x120, v = 0 
            OP_CLC_IMP,         
            OP_LDA_IMM, 0xD0,
            OP_ADC_IMM, 0x50,

    // overflow 0xD0 + 0x90 = 0x160, v = 1 
            OP_CLC_IMP,         
            OP_LDA_IMM, 0xD0,
            OP_ADC_IMM, 0x90,            

    // overflow 0xD0 + 0xD0 = 0x1A0, v = 0 
            OP_CLC_IMP,         
            OP_LDA_IMM, 0xD0,
            OP_ADC_IMM, 0xD0,

    // overflow 0xD0 + 0xD0 = 0x1A0, v=0
            OP_LDA_IMM, 0xD0,
            OP_STA_ZP, 0x78,
            OP_CLC_IMP,
            OP_ADC_ZP, 0x78,

            OP_LDA_IMM, 0xAA,       // 2 cycles
            OP_STA_ZP,  0x00,       // 3 cycles
            OP_LDA_ZP,  0x00,       // 3 cycles
            OP_LDX_ZP,  0x00,       // 3 cycles
            OP_LDY_ZP,  0x01,       // 3 cycles
            OP_STA_ABS, 0x34, 0x12, // 4 cycles
            OP_ADC_IMM, 0x03,       // 2 cycles
            OP_STA_ABS, 0x35, 0x12, // 4 cycles

            OP_LDX_ABS, 0x35, 0x12, // 4 cycles
            OP_LDY_ABS, 0x35, 0x12, // 4 cycles

            OP_CLC_IMP,
            OP_BCC_REL, 0x02,

            OP_BRK_IMP,            // 7 cycles
            OP_JMP_ABS, 0x00, 0x01, // 3 cycles
        },
        .program_size = sizeof(test.program),
        .program_address = 0x0100,
        .setup = [](cpu_state *cpu) {
            // Initial setup if needed
        },
        .assertions = [](cpu_state *cpu) {
            // Assertions to check the final state
            assert(cpu->a_lo == 0xAE);
            assert(cpu->C == 0);
            assert(raw_memory_read(cpu, 0x0000) == 0xAA);
            assert(raw_memory_read(cpu, 0x1234) == 0xAA);
            assert_cycles(cpu, 150);
        }
    };

    run_test(test);
}