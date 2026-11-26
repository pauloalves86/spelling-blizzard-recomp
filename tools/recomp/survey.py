import json, sys
from collections import Counter
from ne import NEFile, sel_of
from x86 import Decoder, DecodeError
n = NEFile('../../original/BLIZZARD.EXE')
idx = json.load(open('../../build/decomp/decomp_index.json'))['functions']
entries = {}
for f in idx:
    s, o = f['entry'].split(':')
    seg = (int(s, 16) - 0x1000) // 8 + 1
    if seg in n.segments and not n.segments[seg].is_data:
        entries.setdefault(seg, set()).add(int(o, 16))
ops = Counter(); errs = []; ind = Counter(); ints=Counter()
for seg, ents in entries.items():
    dec = Decoder(n.segments[seg].data)
    seen = set(); work = list(ents)
    while work:
        a = work.pop()
        while a not in seen:
            seen.add(a)
            try:
                i = dec.decode(a)
            except (DecodeError, IndexError) as e:
                errs.append((seg, a, str(e))); break
            key = i.op + ('.' + str(i.opsize) if i.opsize == 4 else '')
            if any(getattr(o,'size',0)==4 for o in i.ops): key += '.32'
            ops[key] += 1
            if i.op in ('jmp_ind','call_ind','jmpf_ind','callf_ind'): ind[(i.op,str(i.ops[0]))]+=1
            if i.op=='int': ints[i.ops[0].val]+=1
            if i.target is not None and i.op != 'call':
                work.append(i.target)
            if i.op == 'call': work.append(i.target)
            if i.op in ('jmp', 'ret', 'retf', 'jmp_ind', 'jmpf_ind', 'jmpf', 'iret', 'hlt'):
                break
            a += i.length
print(sorted(ops.items(), key=lambda x: -x[1]))
print('errors', len(errs), errs[:10])
print('indirect', ind.most_common(40))
print('ints', ints)
