#!/usr/bin/env python3
"""gen_tk.py — 两遍汇编器 + 地址 patching，生成 TokiOS ring3 程序"""

import struct

CODE_BASE = 0x200010

def addr32(val): return bytes([0xBB, (val>>0)&0xFF, (val>>8)&0xFF, (val>>16)&0xFF, (val>>24)&0xFF])
def mov_edi(val): return bytes([0xBF, (val>>0)&0xFF, (val>>8)&0xFF, (val>>16)&0xFF, (val>>24)&0xFF])
def mov_esi(val): return bytes([0xBE, (val>>0)&0xFF, (val>>8)&0xFF, (val>>16)&0xFF, (val>>24)&0xFF])
def puts_cdecl(addr): return addr32(addr) + b'\xB8\x01\x00\x00\x00\xCD\x80'
def getch(): return b'\xB8\x02\x00\x00\x00\xCD\x80'
def tfs_list(): return b'\xB8\x05\x00\x00\x00\xCD\x80'
def sys_exit(): return b'\xB8\x09\x00\x00\x00\xCD\x80'

class Asm:
    def __init__(self):
        self.code = bytearray()
        self.labels = {}
        self.data = bytearray()
        self.data_labels = {}
        self.fixups = []  # (code_offset, kind, label)

    def L(self, name):
        self.labels[name] = len(self.code)

    def emit(self, bs):
        self.code += bs

    def S(self, name, s):
        self.data_labels[name] = len(self.data)
        self.data += s.encode('ascii') + b'\x00'

    def B(self, name, nbytes, val=0):
        self.data_labels[name] = len(self.data)
        self.data += bytes([val]) * nbytes

    def emit_jmp8(self, label):
        pos = len(self.code)
        self.code += b'\xEB\x00'
        self.fixups.append((pos + 1, 'rel8', label))

    def emit_je8(self, label):
        pos = len(self.code)
        self.code += b'\x74\x00'
        self.fixups.append((pos + 1, 'rel8', label))

    def emit_jne8(self, label):
        pos = len(self.code)
        self.code += b'\x75\x00'
        self.fixups.append((pos + 1, 'rel8', label))

    def emit_addr32(self, label):
        """emit placeholder mov ebx, XXXXXXXX + record fixup"""
        pos = len(self.code)
        self.code += b'\x00\x00\x00\x00\x00'
        self.fixups.append((pos, 'addr32_placeholder', label))

    def emit_edi_addr(self, label):
        pos = len(self.code)
        self.code += b'\x00\x00\x00\x00\x00'
        self.fixups.append((pos, 'edi_addr', label))

    def emit_esi_addr(self, label):
        pos = len(self.code)
        self.code += b'\x00\x00\x00\x00\x00'
        self.fixups.append((pos, 'esi_addr', label))

    def emit_abs_addr(self, label):
        """emit placeholder for 32-bit ABSOLUTE addr in instruction body"""
        pos = len(self.code)
        self.code += b'\x00\x00\x00\x00'
        self.fixups.append((pos, 'abs32', label))

    def emit_mov_edi_abs32(self, label):
        """emit A2 xx xx xx xx — mov [abs32], al"""
        pos = len(self.code)
        self.code += b'\xA2\x00\x00\x00\x00'
        self.fixups.append((pos + 1, 'abs32', label))

    def emit_mov_ebx_abs32(self, label):
        """emit BB xx xx xx xx — mov ebx, imm32"""
        pos = len(self.code)
        self.code += b'\xBB\x00\x00\x00\x00'
        self.fixups.append((pos + 1, 'abs32', label))

    pass  # D() is now handled via resolve()

    def finalize(self):
        code_sz = len(self.code)
        data_labels = self.data_labels

        def resolve_abs(label_name):
            """返回标签的绝对虚拟地址"""
            if label_name in self.labels:
                return CODE_BASE + self.labels[label_name]
            elif label_name in data_labels:
                return CODE_BASE + code_sz + data_labels[label_name]
            else:
                raise KeyError(label_name)

        for (pos, kind, label) in self.fixups:
            if kind == 'rel8':
                target_off = self.labels[label]
                rel = target_off - (pos + 1)
                self.code[pos] = rel & 0xFF
            else:
                target = resolve_abs(label)
                if kind == 'addr32_placeholder':
                    self.code[pos]   = 0xBB
                    self.code[pos+1] = (target>>0) & 0xFF
                    self.code[pos+2] = (target>>8) & 0xFF
                    self.code[pos+3] = (target>>16) & 0xFF
                    self.code[pos+4] = (target>>24) & 0xFF
                elif kind == 'edi_addr':
                    self.code[pos]   = 0xBF
                    self.code[pos+1] = (target>>0) & 0xFF
                    self.code[pos+2] = (target>>8) & 0xFF
                    self.code[pos+3] = (target>>16) & 0xFF
                    self.code[pos+4] = (target>>24) & 0xFF
                elif kind == 'esi_addr':
                    self.code[pos]   = 0xBE
                    self.code[pos+1] = (target>>0) & 0xFF
                    self.code[pos+2] = (target>>8) & 0xFF
                    self.code[pos+3] = (target>>16) & 0xFF
                    self.code[pos+4] = (target>>24) & 0xFF
                elif kind == 'abs32':
                    self.code[pos]   = (target>>0) & 0xFF
                    self.code[pos+1] = (target>>8) & 0xFF
                    self.code[pos+2] = (target>>16) & 0xFF
                    self.code[pos+3] = (target>>24) & 0xFF

        return bytes(self.code) + bytes(self.data)


def build():
    a = Asm()

    # 数据段（字符串 + 缓冲区）
    a.S('welcome', 'TokiShell r3')
    a.S('prompt', '\n> ')
    a.S('unkmsg', 'Unknown')
    a.S('helpmsg', 'help - cmds: ls, help, exit')
    a.S('exitmsg', 'exit - bye!')
    a.S('nl_char', '\n')   # "\n\0" = {0x0A,0x00}
    a.B('cmd_char', 2)   # echo tmp: [0]=char,[1]=\0
    a.B('cmd_buf', 64)

    # ====== 代码段 ======
    a.L('_start')
    a.emit_addr32('welcome')  # mov ebx, placeholder → patched
    a.emit(b'\xB8\x01\x00\x00\x00\xCD\x80')  # puts

    a.L('loop')
    a.emit_addr32('prompt')
    a.emit(b'\xB8\x01\x00\x00\x00\xCD\x80')

    # 读字符到 cmd_buf
    a.emit_edi_addr('cmd_buf')

    a.L('readc')
    a.emit(getch())           # al = char

    # --- 退格 0x08 ---
    a.emit(b'\x3C\x08')     # cmp al, 8
    a.emit_jne8('chk_cr')
    # edi <= cmd_buf? → ignore
    a.emit(b'\x81\xFF')    # cmp edi, imm32
    a.emit_abs_addr('cmd_buf')
    a.emit_je8('readc')     # edi==cmd_buf → ignore
    a.emit(b'\x4F')          # dec edi
    a.emit(b'\xC6\x07\x00') # mov [edi], 0 (清缓冲区)
    # SYS_BACKSPACE: kernel handles VGA erase
    a.emit(b'\xB8\x0B\x00\x00\x00\xCD\x80')
    a.emit_jmp8('readc')

    a.L('chk_cr')
    a.emit(b'\x3C\x0D')     # cmp al, CR
    a.emit_je8('done_read')
    a.emit(b'\x3C\x0A')    # cmp al, LF
    a.emit_je8('done_read')

    # 回显: al→存cmd_char→SYS_COUT→从cmd_char恢复→存cmd_buf
    a.emit(b'\xA2')        # mov [cmd_char], al
    a.emit_abs_addr('cmd_char')
    a.emit_mov_ebx_abs32('cmd_char')
    a.emit(b'\xB8\x01\x00\x00\x00\xCD\x80')
    a.emit(b'\xA0')        # mov al, [cmd_char] — 不用 bl，避免 ebx 覆盖
    a.emit_abs_addr('cmd_char')
    # 存入 [edi]
    a.emit(b'\x88\x07')    # mov [edi], al
    a.emit(b'\x47')         # inc edi
    a.emit_jmp8('readc')

    a.L('done_read')
    a.emit(b'\xC6\x07\x00') # null at [edi]
    # echo newline
    a.emit_mov_ebx_abs32('nl_char')
    a.emit(b'\xB8\x01\x00\x00\x00\xCD\x80')

    # 解析命令
    a.emit_esi_addr('cmd_buf')

    # --- "ls\0" ---
    a.emit(b'\x80\x3E\x6C')  # cmp [esi], 'l'
    a.emit_jne8('chk_help')
    a.emit(b'\x80\x7E\x01\x73')  # cmp [esi+1], 's'
    a.emit_jne8('chk_help')
    a.emit(b'\x80\x7E\x02\x00')  # cmp [esi+2], 0
    a.emit_je8('do_ls')

    a.L('chk_help')
    a.emit(b'\x80\x3E\x68')  # 'h'
    a.emit_jne8('chk_exit')
    a.emit(b'\x80\x7E\x01\x65')  # 'e'
    a.emit_jne8('chk_exit')
    a.emit(b'\x80\x7E\x02\x6C')  # 'l'
    a.emit_jne8('chk_exit')
    a.emit(b'\x80\x7E\x03\x70')  # 'p'
    a.emit_jne8('chk_exit')
    a.emit(b'\x80\x7E\x04\x00')
    a.emit_je8('do_help')

    a.L('chk_exit')
    a.emit(b'\x80\x3E\x65')  # 'e'
    a.emit_jne8('unknown')
    a.emit(b'\x80\x7E\x01\x78')  # 'x'
    a.emit_jne8('unknown')
    a.emit(b'\x80\x7E\x02\x69')  # 'i'
    a.emit_jne8('unknown')
    a.emit(b'\x80\x7E\x03\x74')  # 't'
    a.emit_jne8('unknown')
    a.emit(b'\x80\x7E\x04\x00')
    a.emit_je8('do_exit')

    a.L('unknown')
    a.emit_addr32('unkmsg')
    a.emit(b'\xB8\x01\x00\x00\x00\xCD\x80')
    a.emit_jmp8('ret2loop')

    a.L('do_ls')
    a.emit(tfs_list())
    a.emit_jmp8('ret2loop')

    a.L('do_help')
    a.emit_addr32('helpmsg')
    a.emit(b'\xB8\x01\x00\x00\x00\xCD\x80')
    a.emit_jmp8('ret2loop')

    a.L('do_exit')
    a.emit(sys_exit())

    # ret2loop: 输出提示符后跳回 readc
    a.L('ret2loop')
    a.emit_mov_ebx_abs32('prompt')
    a.emit(b'\xB8\x01\x00\x00\x00\xCD\x80')
    rel = a.labels['loop'] - (a.labels['ret2loop'] + 5)
    a.emit(bytes([0xE9]) + struct.pack('<i', rel))

    a.L('_end_code')

    full = a.finalize()
    total = len(full)
    code_sz = len(a.code)

    print(f'// shell_code 大小: {total} bytes (code={code_sz}, data={len(a.data)})')
    print(f'// 地址范围: 0x{CODE_BASE:X} - 0x{CODE_BASE + total:X}')
    for k in a.data_labels:
        abs_addr = CODE_BASE + code_sz + a.data_labels[k]
        print(f'//   {k} @ 0x{abs_addr:X}')
    print('static const char shell_code[] = {')
    for i in range(0, len(full), 16):
        chunk = full[i:i+16]
        line = ','.join(f'0x{b:02X}' for b in chunk)
        comma = ',' if i + 16 < len(full) else ''
        print(f'    {line}{comma}')
    print('};')

if __name__ == '__main__':
    build()
