import ida_hexrays, ida_funcs, idc
for a in [0x82614d98,0x8261244c,0x825fa394,0x827d8a20]:
    f=ida_funcs.get_func(a)
    txt = str(ida_hexrays.decompile(f))
    print('---',hex(a),idc.get_name(a),'---')
    print(txt)
