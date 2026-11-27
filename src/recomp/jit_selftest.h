/* Exercises the CPU helpers the generated code inlines (cpu.h): arithmetic,
 * flags, shifts and rotates, sign extension, a switch, and memory and stack
 * access through a selector. jit.c builds it twice, with the host compiler
 * and in memory with TinyCC, and compares the two (--self-test). */
uint32_t selftest_mix(uint32_t a, uint32_t b)
{
    uint32_t h = 2166136261u;
#define MIX(v) (h = (h ^ (uint32_t)(v)) * 16777619u, h = (h ^ get_flags()) * 16777619u)
    uint16_t x = (uint16_t)a, y = (uint16_t)b;
    uint8_t p = (uint8_t)a, q = (uint8_t)(b >> 8);
    unsigned c = (b >> 16) & 31;
    cpu.cf = a & 1;
    MIX(add16(x, y)); MIX(adc16(x, y)); MIX(sub16(x, y)); MIX(sbb16(x, y));
    MIX(and16(x, y)); MIX(or16(x, y)); MIX(xor16(x, y));
    MIX(inc16(x)); MIX(dec16(y)); MIX(neg16(x));
    MIX(add8(p, q)); MIX(adc8(p, q)); MIX(sub8(p, q)); MIX(sbb8(p, q)); MIX(neg8(p)); MIX(inc8(q)); MIX(dec8(p));
    MIX(add32(a, b)); MIX(adc32(a, b)); MIX(sub32(a, b)); MIX(sbb32(a, b)); MIX(neg32(a));
    MIX(shl16(x, c)); MIX(shr16(x, c)); MIX(sar16(x, c)); MIX(rol16(x, c)); MIX(ror16(x, c));
    MIX(rcl16(x, c)); MIX(rcr16(x, c));
    MIX(shl8(p, c)); MIX(shr8(p, c)); MIX(sar8(p, c)); MIX(rol8(p, c)); MIX(ror8(p, c)); MIX(rcl8(p, c));
    MIX(rcr8(p, c));
    MIX(shl32(a, c)); MIX(shr32(a, c)); MIX(sar32(a, c)); MIX(rol32(a, c)); MIX(ror32(a, c));
    MIX(shld16(x, y, c)); MIX(shrd16(x, y, c)); MIX(shld32(a, b, c)); MIX(shrd32(a, b, c));
    MIX((uint16_t)(int16_t)(int8_t)p); MIX((uint32_t)(int32_t)(int16_t)x);
    switch (x & 7) {
    case 0: MIX(1); break;
    case 3: MIX(3); /* fallthrough */
    case 5: MIX(5); break;
    default: MIX(9); break;
    }
    set_flags((uint16_t)b);
    MIX(PF());
    /* memory and the stack, through selector 0x10 (base 0) */
    SS = 0x10;
    SP = 0x100;
    PUSH16(x);
    PUSH32(a);
    MIX(POP32()); MIX(POP16());
    WW(SS, 0x40, y); WB(SS, 0x43, p); WD(SS, 0x50, a ^ b);
    MIX(RW(SS, 0x40)); MIX(RD(SS, 0x40)); MIX(RB(SS, 0x43)); MIX(RD(SS, 0x50)); MIX(SP);
#undef MIX
    return h;
}
