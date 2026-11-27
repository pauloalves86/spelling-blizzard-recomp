/* Compiled in front of the generated code when the game is compiled in memory
 * (jit.c), in place of the system headers: there are none at run time. cpu.h
 * and api_decls.h follow it, with their #include lines dropped. */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef __SIZE_TYPE__ size_t;
void *memcpy(void *dst, const void *src, size_t n);
