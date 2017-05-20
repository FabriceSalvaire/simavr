/*
 * In order to maximize performance and parallelism, the AVR uses a Harvard architecture with
 * separate memories and buses for program and data.
 *
 * Instructions in the program memory are executed with a single level pipelining.  While one
 * instruction is being executed, the next instruction is pre-fetched from the program memory. This
 * concept enables instructions to be executed in every clock cycle. The program memory is In-System
 * Reprogrammable Flash memory.
 *
 * The fast-access Register File contains 32 × 8-bit general purpose working registers with a single
 * clock cycle access time. This allows single-cycle Arithmetic Logic Unit (ALU) operation. In a
 * typical ALU operation, two operands are output from the Register File, the operation is executed,
 * and the result is stored back in the Register File in one clock cycle.
 *
 * Six of the 32 registers can be used as three 16-bit indirect address register pointers for Data
 * Space addressing – enabling efficient address calculations. One of the these address pointers can
 * also be used as an address pointer for look up tables in Flash program memory. These added
 * function registers are the 16-bit X-, Y-, and Z-register.
 *
 */
