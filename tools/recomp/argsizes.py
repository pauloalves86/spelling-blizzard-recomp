"""Infer bytes pushed before each imported API call (straight-line push runs)."""
import sys
from collections import defaultdict
sys.argv = ['gen.py', '/dev/null']
import gen
from x86 import Reg, Mem, Imm, SReg
prog = gen.Program()
res = defaultdict(set)
for seg, ents in prog.entries.items():
    for e in ents:
        f = gen.Func(prog, seg, e); f.decode()
        addrs = sorted(a for a in f.ins if not isinstance(f.ins[a], tuple))
        for k, a in enumerate(addrs):
            i = f.ins[a]
            if i.op != 'callf': continue
            r = prog.reloc.get((seg, a + i.imm_off))
            if not r or r[0] != 'import': continue
            name = gen.EXPORTS.get(r[1], (str(r[1]), 0))[0]
            n = 0; j = k - 1
            while j >= 0:
                p = f.ins[addrs[j]]
                if addrs[j] + p.length != addrs[j+1] or addrs[j+1] in f.labels and j+1 != k and False: break
                if p.op == 'push':
                    o = p.ops[0]
                    n += 4 if (getattr(o,'size',2) == 4 or (isinstance(o, Imm) and p.opsize == 4)) else 2
                elif p.op in ('call','callf','callf_ind','jmp','ret','retf') or (p.op=='add' and str(p.ops[0])=='sp') or (p.op=='sub' and str(p.ops[0])=='sp'):
                    break
                j -= 1
            res[name].add(n)
for k in sorted(res): print(k, sorted(res[k]))
