# TokiOS 架构文档

> 2026-06-17 · commit `4c2634a`  
> 22 个源文件、~2.5k 行 C、~200 行 ASM、13 个系统调用

---

## 一、总览

TokiOS 是一个 x86 教学内核。跑在 QEMU 上，单核、纯 32 位保护模式、分页启用。没有 POSIX、没有 fork、一切是文件名不是 fd。

**当前完成度：**

- ✅ Multiboot / PVH 启动
- ✅ GDT（ring0 + ring3 + TSS, 6 条目）
- ✅ IDT（256 条目, PIC 重映射, 异常 0-31, IRQ 0-15）
- ✅ 分页（恒等映射 0-4MB + 用户 2MB-3MB）
- ✅ 键盘中断 → scancode→ASCII → VGA 回显 + 环形缓冲
- ✅ Ring0 shell（15 命令, 函数指针表分发）
- ✅ Ring3 跳转（`proc_start` 纯汇编 iret）
- ✅ `int 0x80` trap gate（13 个系统调用）
- ✅ `tokios.tk` ring3 shell（ls / help / exit / 退格）
- ✅ TokiFS v0.2（16 槽, 1MB RAM 盘, 删除紧凑回收）
- ✅ 多用户认证（SHA-256, /etc/passwd, 输错 3 次 30s 冻结）
- ✅ CPU 异常 0-31 dump 寄存器 → halt
- ✅ TSS IOPB（ring3 IO 全禁）

---

## 二、启动流程

```
QEMU -kernel tokios.elf
  │
  ├── Multiboot1 头 (.multiboot)    ← QEMU <9
  ├── Multiboot2 头 (.multiboot2)   ← QEMU 9-10
  └── PVH ELF Note (.note.Xen)      ← QEMU 11, 唯一认的协议
       │
       ▼
   _start (boot.S)
       │ mov $stack_top, %esp
       │ call kernel_main
       ▼
   kernel_main (kernel.c)
       │ 1. 清屏 (VGA 0xB8000)
       │ 2. 打印 "TokiOS booted."
       │ 3. gdt_install()       ← 6 条目 GDT
       │ 4. tss_init()          ← TSS esp0 + IOPB
       │ 5. paging_init()       ← 恒等映射 0-4MB, 开分页
       │ 6. page_map_user()     ← 映射 2MB 为 user 页
       │ 7. idt_install()       ← IDT + PIC + sti
       │ 8. keyboard_init()     ← (空, hook 留白)
       │ 9. tfs_init()          ← TokiFS 清零
       │ 10. auth_init()        ← 读/建 /etc/passwd
       │ 11. tfs_create("tokios.tk", ...)   ← 嵌入 ring3 shell
       │ 12. 打印 "TokiOS> "
       │ 13. for(;;) hlt        ← 等待中断
       ▼
   [键盘中断 → keyboard_handler → shell_run → ...]
```

---

## 三、内存布局

```
物理地址          内容
────────────────────────────────────
0x000000 – 0x0FFFFF   前 1MB: IVT, BDA, BIOS
0x100000              页目录 (1024×4B, 4KB aligned)
0x101000              页表 [0] (恒等映射 0–4MB)
0x1xxxxx              内核 .text / .rodata / .data / .bss
0x200000 – 0x2FFFFF   用户空间 (代码+数据, 1MB)
0x300000 – 0x3FFFFF   TokiFS 数据区 (1MB)
0x9FC00               TSS esp0 (ring0 内核栈)
0xB8000               VGA 文本缓冲区 (80×25 字符)
```

GDT 选择子速查：
| 选择子 | 段 | DPL |
|--------|-----|-----|
| `0x08` | Ring0 代码 | 0 |
| `0x10` | Ring0 数据 | 0 |
| `0x1B` | Ring3 代码 | 3 |
| `0x23` | Ring3 数据 | 3 |
| `0x28` | TSS | 0 |

---

## 四、文件详解

### 4.1 `boot.S` — 启动汇编

**职责：** 三段启动头 + 中断/异常/系统调用存根 + `proc_start`

```asm
# 三段启动头（兼容 QEMU 多版本）
.section .multiboot    → Multiboot1 魔数
.section .multiboot2   → Multiboot2 魔数
.section .note.Xen     → PVH ELF Note（QEMU 11 唯一支持）
```

**IRQ 存根宏：**
```asm
.macro IRQ num, handler
\handler:
    cli
    pushl $0           # 无错误码, 推 dummy
    pushl $\num        # 中断号
    jmp irq_common_stub
.endm
```
展开为 `irq0`–`irq15`。每个存根做同样的事：关中断 → 推 dummy err → 推中断号 → 跳到公共存根。

**`irq_common_stub` — 中断帧保存：**
```
栈帧布局（push 顺序 = 从高到低）:
  ss, useresp, eflags, cs, eip    ← CPU 自动推（有特权级切换时）
  err_code, int_no                ← 存根推
  gs, fs, es, ds
  edi, esi, ebp, esp, ebx, edx, ecx, eax
  ↑ 这就是 struct regs 的内存布局
```

**`proc_start` — ring0→ring3 跳转：**
```asm
proc_start:
    pushl %ebp
    movl %esp, %ebp
    movl 0x8(%ebp), %eax     # entry (EIP)
    movl 0xc(%ebp), %ebx     # stack_top (ESP)
    pushl $0x23              # SS = ring3 data
    pushl %ebx               # ESP
    pushfl                   # EFLAGS
    pushl $0x1B              # CS = ring3 code
    pushl %eax               # EIP
    iret                     # 弹出 5 个值, 跳入 ring3
```
iret 弹出顺序：EIP → CS → EFLAGS → ESP → SS。正好是 ring3 的所有寄存器。

---

### 4.2 `kernel.c` — 内核入口

**职责：** 初始化链 + 嵌入 ring3 shell 二进制 + 主循环

```c
void kernel_main(void)
{
    // 1-2. 清屏 + 欢迎
    for (int i = 0; i < 80 * 25; i++)
        vga[i] = (0x0F << 8) | ' ';

    // 3-10. 初始化链（顺序严格）
    gdt_install();        // 必须先于 tss_init
    tss_init();           // 设 esp0 + IOPB
    paging_init();        // 恒等映射
    page_map_user(0x200000, 0x200000);  // 用户页
    idt_install();        // 设中断 + sti
    keyboard_init();
    tfs_init();
    auth_init();          // /etc/passwd

    // 11. 嵌入 ring3 shell
    // 在栈上构建 16 字节 .tk 头 + shell_code
    static char shell_tk[16 + sizeof(shell_code)];
    // 头: "toki\0\0" + entry(0x200000) + code_size + flags(0)
    shell_tk[0]='t'; shell_tk[1]='o'; shell_tk[2]='k'; shell_tk[3]='i';
    ...
    tfs_create("shell.tk", shell_tk, sizeof(shell_tk));

    // 13. 等中断（内核靠键盘中断活）
    for(;;) __asm__ volatile("hlt");
}
```

**`shell_code[]` — ring3 shell.tk 的机器码**
由 `tools/gen_tk.py` 生成，嵌入内核。当前是 v7 版本（退格 = SYS_BACKSPACE）。

---

### 4.3 `gdt.c` + `gdt.h` — 全局描述符表

**职责：** 段寄存器 & 特权级隔离的基础

```c
// gdt[6] 布局:
// [0] NULL       — 必须, 访问 0x00 即 #GP
// [1] Ring0 代码  — 0x08, DPL=0, 4GB
// [2] Ring0 数据  — 0x10, DPL=0, 4GB
// [3] Ring3 代码  — 0x1B, DPL=3, 1MB
// [4] Ring3 数据  — 0x23, DPL=3, 1MB
// [5] TSS        — 0x28, DPL=0, 硬件任务状态段
```

**TSS 结构体字段序：** `iopb` 必须在偏移 0x64（x86 硬件在此读 I/O Map Base）。如果 sizeof(tss) < 0x64，iopb 位置会偏，`inb`/`outb` 不被拦截。

**TSS 关键字段：**
```c
tss.ss0  = 0x10;     // ring0 数据段
tss.esp0 = 0x9FC00;  // 内核栈顶（ring3→ring0 切换时 CPU 自动加载）
tss.iopb = sizeof(tss_t);  // IOPB 偏移 ≥ 结构体大小 = 全部 IO 端口设 1 → 禁止 ring3 IO
```

---

### 4.4 `idt.c` + `idt.h` — 中断描述符表

**职责：** 注册 256 个门（异常 / IRQ / trap）

```c
void idt_install()
{
    // 1. 注册 32 个异常存根 (exc0–exc31), 0x8E = interrupt gate, DPL=0
    for(int i = 0; i < 32; i++)
        idt_set_gate(i, exc_addrs[i], 0x08, 0x8E);

    // 2. PIC 重映射: IRQ 0-7 → 0x20-0x27, IRQ 8-15 → 0x28-0x2F
    //    （防止 IRQ 和 CPU 异常号冲突）
    outb(PIC1_DATA, 0xFD);  // 0xFD = 11111101, 只开 IRQ1 (键盘)
    outb(PIC2_DATA, 0xFF);  // 全关

    // 3. 注册 16 个 IRQ 存根 (irq0–irq15), 映射到 IDT[32]-IDT[47]
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);  // 键盘

    // 4. 系统调用门: int 0x80, 0xEF = trap gate, DPL=3
    idt_set_gate(0x80, (uint32_t)isr128, 0x08, 0xEF);

    // 5. 加载 IDT + 开中断
    idt_flush((uint32_t)&idtp);
    __asm__ volatile("sti");
}
```

**为什么用 trap gate (0xEF) 而不是 interrupt gate (0x8E)？**
- Interrupt gate → `cli` 隐式在入口执行 → 禁止嵌套
- Trap gate → 不自动关中断 → 允许更高优先级中断打断系统调用
- 当前 TokiOS 单核、无非抢占，两者行为一样。trap gate 为未来留空间

---

### 4.5 `isr.c` + `isr.h` — 中断分派

**职责：** IRQ 分派 + 异常处理 + 寄存器结构体定义

```c
// struct regs 内存布局 (与 boot.S push 顺序完全一致):
struct regs {
    uint32_t ds, es, fs, gs;     // 段寄存器
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  // pusha
    uint32_t int_no, err_code;   // 中断号 + 错误码
    uint32_t eip, cs, eflags, useresp, ss;  // CPU 自动推
};
```

**IRQ 分派：**
```c
void irq_handler(struct regs *r)
{
    // EOI 发往 PIC (级联: 先 slave 后 master)
    if (r->int_no >= 8) outb(0xA0, 0x20);
    outb(0x20, 0x20);

    // 只有 IRQ1 (键盘) 有 handler
    if(r->int_no == 1) keyboard_handler();
}
```

**异常处理：**
```c
void exception_handler(struct regs *r)
{
    // dump: 异常号 + EIP + CS + ERR + EAX-EBX-ECX-EDX + CR2
    // 然后死循环 halt，不恢复
}
```

---

### 4.6 `keyboard.c` — 键盘 & 光标

**职责：** 键盘中断处理 + VGA 文本输出 + 环形缓冲 + 滚屏

**三层隔离：**

```
键盘中断 → keyboard_handler()
  │
  ├── 无论什么 ring → 推入 keybuf 环形缓冲 (ring3 getch 使用)
  │
  └── if (!in_ring3) → ring0 shell 输入处理
       │ 退格 → cursor_backspace + input_len--
       │ 回车 → input[input_len]='\0' → shell_run()
       │ 普通字符 → 回显 + input 追加
       │
       └── if (in_ring3) → 只推 ringbuf, 不碰 ring0 shell
```

**`cursor_write`** — 唯一的文本输出路径：
```c
void cursor_write(const char *s) {
    for(; *s; s++) {
        case '\n': cursor = (cursor/80 + 1)*80; scroll if ≥ 80*25;
        case '\b': cursor--;
        default:   VIDEO[cursor*2]=*s; VIDEO[cursor*2+1]=0x0F; cursor++;
    }
}
```

**`scroll_up()`** — 自动滚屏（超 25 行时触发）：
```c
// 第 2–25 行上移, 清空第 25 行
for (int i=0; i<80*24; i++)
    VIDEO[i*2] = VIDEO[(i+80)*2];
```

**`getch()` — ring3 读键（阻塞）:**
```c
char getch(void) {
    while(keybuf_read == keybuf_write)   // 环形缓冲空 → 等中断
        __asm__ volatile("sti; hlt; cli");
    return keybuf[keybuf_read++];
}
```

**`cursor_backspace()` — 真·退格擦除：**
```c
// 区别于 cursor_write 的 \b（只回退光标不擦字符）
// 这里真正在 VGA 上擦掉那个字符
VIDEO[cursor*2] = ' ';   // 写空格覆盖
```

---

### 4.7 `shell.c` + `shell.h` — Ring0 命令行

**职责：** 命令解析 + 分发

**分发机制 — 函数指针表：**
```c
typedef struct {
    const char *name;
    void (*func)(int argc, const char *argv[]);
    const char *help;
} cmd_t;

static const cmd_t cmd_table[] = {
    {"ls",     cmd_ls,     "list files"},
    {"run",    cmd_run,    "run <file>"},
    {"cat",    cmd_cat,    "cat <file>"},
    {"touch",  cmd_touch,  "touch <file> <data>"},
    {"rm",     cmd_rm,     "rm <file>"},
    {"clear",  cmd_clear,  "clear screen"},
    {"info",   cmd_info,   "show cpu regs"},
    {"help",   cmd_help,   "show this"},
    {"login",  cmd_login,  "login <user> <pass>"},
    {"logout", cmd_logout, "logout"},
    {"whoami", cmd_whoami, "whoami"},
    {"passwd", cmd_passwd, "change my password"},
    {"useradd",cmd_useradd,"useradd <user> <pass>"},
    {"crash",  cmd_crash,  "trigger exception 0"},
    {0, 0, 0}
};
```

**参数解析 `parse_args`：**
```c
// "cat  shell.tk"  →  argv[0]="cat", argv[1]="shell.tk"
// 原地修改 input 字符串：空格换成 '\0'
```

**`cmd_ls` / `cmd_cat` / `cmd_run`** 分别调用 `tfs_list()` / `tfs_read()` / `proc_load()`。

**`cmd_info`** 读 CR0 / CR2 / CR3 打印：
```c
__asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
```

---

### 4.8 `syscall.c` + `syscall.h` — 系统调用分发

**职责：** `int 0x80` → `switch(eax)` → 调用对应内核函数

```c
void syscall_handler(struct regs *r)
{
    if(r->eax < 1 || r->eax > SYS_MAX) return;   // 边界检查
    switch(r->eax) {
        case SYS_COUT:  cursor_write((const char*)r->ebx);  break;
        case SYS_GETCH: r->eax = getch();                   break;
        // ...
        case SYS_EXIT:  // 恢复内核栈, sti, leave, ret
        case SYS_LOGIN: r->eax = auth_login(...);           break;
        case SYS_EXEC:  r->eax = proc_load(...);            break;
    }
}
```

**调用号表：**

| # | 名 | ebx | ecx | edx | 返回 eax |
|---|------|-----|-----|-----|----------|
| 1 | COUT | 字符串指针 | — | — | — |
| 2 | GETCH | — | — | — | 字符 |
| 3 | CUR_SET | 位置 | — | — | — |
| 4 | CUR_GET | — | — | — | 光标位置 |
| 5 | TFS_LIST | — | — | — | — |
| 6 | TFS_READ | 文件名 | 缓冲区 | buf 大小 | 读到的字节 |
| 7 | TFS_CREATE | 文件名 | 数据 | 长度 | 0=ok |
| 8 | TFS_DELETE | 文件名 | — | — | 0=ok |
| 9 | EXIT | — | — | — | 不返回 |
| 10 | HEX | 值 | — | — | — |
| 11 | BACKSPACE | — | — | — | — |
| 12 | LOGIN | 用户名 | 密码 | — | 1=ok |
| 13 | EXEC | 文件名 | — | — | 不返回/0=失败 |

**SYS_EXIT 实现细节：**
```c
case SYS_EXIT:
    in_ring3 = 0;
    __asm__ volatile(
        "movl %0, %%esp     \n"   // 恢复内核栈
        "movl %%esp, %%ebp  \n"
        "sti                \n"
        "xorl %%eax, %%eax  \n"
        "leave              \n"
        "ret                \n"   // 返回到 kernel_main 的 HLT 循环
        : : "m"(kern_restore_esp)
    );
```

---

### 4.9 `tfs.c` + `tfs.h` — TokiFS v0.2

**职责：** RAM 盘文件系统（16 槽位, 1MB 数据区, 删除回收）

```c
#define MAX_FILES 16       // 最多 16 个文件
#define MAX_FNAME 28       // 文件名最长 28 字符
#define FS_BASE   0x300000 // RAM 盘起始地址
#define FS_SIZE   0x100000 // 1MB

typedef struct {
    char name[MAX_FNAME];
    uint32_t size;          // 文件大小
    uint32_t offset;        // 在数据区的偏移
    uint8_t  used;          // 1=使用中
} file_t;
```

**`tfs_find()` — 统一名字查找：**
```c
static int tfs_find(const char *name) {
    for(int i=0; i<MAX_FILES; i++) {
        if(!files[i].used) continue;
        // 逐字符比较，遇到 name[j]=='\0' 就 break
        for(int j=0; j<MAX_FNAME; j++) {
            if(files[i].name[j] != name[j]) { match=0; break; }
            if(!name[j]) {
                if(files[i].name[j]!=0) match=0;
                break;  // ← break 在外面！之前 tfs_get_size 的 break 在里面是 bug
            }
        }
        if(match) return i;
    }
    return -1;
}
```
被 `tfs_read`、`tfs_delete`、`tfs_get_size` 共用，消灭重复的不一致名字比较。

**`tfs_delete` — 紧凑回收：**
```
删之前:                 删之后:
[A:100B] [B:50B]       [B:50B]
offset=0  offset=100    offset=0 (前移了 100B)
next_free=150           next_free=50
```
```c
int tfs_delete(const char *name) {
    int slot = tfs_find(name);
    uint32_t gap     = files[slot].size;    // 空隙大小
    uint32_t gap_off = files[slot].offset;  // 空隙起始偏移

    // 所有 offset > gap_off 的文件数据前移 gap 字节
    for(int i=0; i<MAX_FILES; i++) {
        if(!files[i].used || i==slot) continue;
        if(files[i].offset > gap_off) {
            memcpy(FS_BASE + files[i].offset - gap,
                   FS_BASE + files[i].offset, files[i].size);
            files[i].offset -= gap;
        }
    }
    files[slot].used = 0;
    next_free -= gap;
}
```

---

### 4.10 `paging.c` + `paging.h` — 内存分页

**职责：** 恒等映射 0–4MB + 用户页

```c
void paging_init(void) {
    // 页表 [0]: 4MB 恒等映射, PAGE_PRESENT + PAGE_RW
    for(int i=0; i<1024; i++)
        first_page_table[i] = (i*4096) | PAGE_PRESENT | PAGE_RW;

    // 页目录 [0] 指向 first_page_table
    page_directory[0] = (uint32_t)first_page_table | PAGE_PRESENT | PAGE_RW;

    // 加载 CR3 + 开分页 (CR0.PG = 1)
    __asm__ volatile("mov %0, %%cr3" : : "r"(page_directory));
    cr0 |= 0x80000000;  // PG bit
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}
```

**`page_map_user` — 标记页为用户可访问：**
```c
void page_map_user(uint32_t vaddr, uint32_t paddr) {
    uint32_t pd_index = vaddr >> 22;           // 页目录索引
    uint32_t pt_index = (vaddr >> 12) & 0x3FF; // 页表索引

    page_directory[pd_index] |= PAGE_PRESENT | PAGE_RW | PAGE_USER;
    first_page_table[pt_index] = (paddr & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW | PAGE_USER;
}
```

当前只用了页表 [0]（4KB 粒度，前 4MB）。未来扩展需要分配更多页表。

**页标志速查：**
| 位 | 名 | 含义 |
|----|-----|------|
| 0 | `PAGE_PRESENT` | 页在物理内存中 |
| 1 | `PAGE_RW` | 可读写（0=只读） |
| 2 | `PAGE_USER` | ring3 可访问（0=仅 ring0） |

---

### 4.11 `proc.c` + `proc.h` — 进程管理

**职责：** 从 TFS 加载 .tk 文件 → 映射用户页 → 跳 ring3

```c
int proc_load(const char *name)
{
    // 1. 获取文件大小, 检查 ≥ 16 字节 (tk_header_t 大小)
    int size = tfs_get_size(name);
    if (size < (int)sizeof(tk_header_t)) return -1;

    // 2. 读文件到 USER_CODE_BASE (0x200000)
    char *hdr_buf = (char*)USER_CODE_BASE;
    int n = tfs_read(name, hdr_buf, size + 1);
    if (n < 0) return -1;

    // 3. 验证 .tk 头部
    tk_header_t *hdr = (tk_header_t *)hdr_buf;
    if (tk_validate(hdr) != 0) return -1;       // magic / code_size / flags
    if (sizeof(tk_header_t) + hdr->code_size != (uint32_t)size) return -1;

    // 4. 清键盘缓冲 + 映射用户页
    keybuf_clear();
    page_map_user(USER_CODE_BASE, USER_CODE_BASE);
    page_map_user(USER_STACK_TOP & 0xFFFFF000, USER_STACK_TOP & 0xFFFFF000);

    // 5. 保存内核栈 → 标记 ring3 → iret 跳转
    __asm__ volatile("movl %%ebp, %0" : "=m"(kern_restore_esp));
    in_ring3 = 1;
    uint32_t final_entry = hdr->entry + sizeof(tk_header_t);  // 跳过 16 字节头
    proc_start(final_entry, USER_STACK_TOP);
    return 0;
}
```

**流程链：**
```
ring0 shell "run shell.tk"
  → proc_load("shell.tk")
    → tfs_get_size → 338
    → tfs_read → 读 339 字节到 0x200000
    → tk_validate → magic=toki ✓, code_size=322 ✓
    → page_map_user (确保 USER page 可访问)
    → proc_start(0x200010, 0x2FFFFC)
      → iret → CS=0x1B, EIP=0x200010, SS=0x23, ESP=0x2FFFFC
        → ring3 代码开始执行
```

---

### 4.12 `tklib.h` — .tk 文件格式

**职责：** 定义 16 字节文件头 + 验证函数

```c
#define TK_MAGIC 0x696B6F74  // "toki" 小端序

typedef struct {
    uint32_t magic;       // 0x696B6F74 — 魔数
    uint32_t entry;       // 入口偏移 (当前固定 0x00200000)
    uint32_t code_size;   // 纯机器码大小
    uint32_t flags;       // 权限 (当前固定 0)
} __attribute__((packed)) tk_header_t;

// .tk 文件 = [16 字节头] + [code_size 字节机器码]
```

`tk_validate` 检查：magic 是否为 `toki`、code_size 是否在 1..1MB 内、flags 是否为零。

---

### 4.13 `auth.c` + `auth.h` — 多用户认证

**职责：** SHA-256 密码哈希 + 用户表 + /etc/passwd 持久化

**数据结构：**
```c
#define MAX_USERS    8
#define MAX_USERNAME 31

typedef struct {
    char name[MAX_USERNAME+1];   // 用户名
    char hash[65];               // SHA-256 的 hex 字符串 (64 字符 + \0)
    int  used;
} user_entry_t;
```

**启动流程：**
```
auth_init()
  │ tfs_read("/etc/passwd", buf, 512)
  ├── n ≤ 0 → buf 为空 → 创建默认用户 tokisora
  │    sha256("tokisora") → hex → users[0].hash
  │    users[0].name = "tokisora"
  │    passwd_sync()  → tfs_create("/etc/passwd", "tokisora:64hex\n", ...)
  │
  └── n > 0 → 解析 "user:64hex\n..." 格式
       逐行扫描, 填 users[uid]

passwd_sync()
  │ 遍历 users[], 拼 "user:hash\n"
  │ tfs_delete("/etc/passwd")
  └ tfs_create("/etc/passwd", buf, pos)
```

**登录防暴力：**
```
auth_login("tokisora", "wrong")
  → fail_cnt: 0→1 → "wrong password (1/3)"
  → fail_cnt: 1→2 → "wrong password (2/3)"
  → fail_cnt: 2→3 → "3 fails => 30s lockout"
  → busy-wait 30s (for i<300, for j<10000000, pause)
  → fail_cnt=0, auth_locked=0
```

密码存储为 SHA-256 的 **小写 hex 字符串**（64 字符），不存明文或二进制。

---

### 4.14 `linker.ld` — 链接脚本

```ld
ENTRY(_start)
SECTIONS {
    . = 1M;               // 从 1MB 开始（跳过 BIOS/IVT）
    .multiboot : { *(.multiboot) }
    .multiboot2 : { *(.multiboot2) }
    .note.Xen : { *(.note.Xen) }    // PVH
    .text : { *(.text) *(.rodata) *(.data) }
    .bss ALIGN(4096) : { *(.bss) *(COMMON) }  // BSS 4KB 对齐
}
```

---

### 4.15 `tools/gen_tk.py` — 两遍汇编器

不在 C 工程中但和 `shell_code[]` 紧密耦合。作用：把 ring3 ASM 源文件汇编成裸机器码 → 粘贴到 `kernel.c` 的 `shell_code[]`。

两遍策略：第一遍算偏移（处理 rel8 跳转），第二遍回填地址。

---

## 五、调用链速查

### 中断完整路径

```
硬件按键盘
  → PIC 发 IRQ1 (int 0x21)
  → CPU 查 IDT[33] → irq1 存根 (boot.S)
    → push dummy + push 33 → jmp irq_common_stub
    → pusha + push 段 → call irq_handler (isr.c)
      → EOI + keyboard_handler() (keyboard.c)
        → inb(0x60) 读 scancode → scan_ascii[sc] 转 ASCII
        → 推 keybuf 环形缓冲
        → if ring0: shell 回显处理
  → popa + addl $8 + iret 回原位
```

### 系统调用完整路径

```
ring3 代码:
    mov $1, %eax      // SYS_COUT
    mov $msg, %ebx    // 字符串指针
    int $0x80
  → CPU 查 IDT[0x80] → isr128 存根 (boot.S)
    → pusha + call syscall_handler (syscall.c)
      → switch(eax):
          case 1: cursor_write(ebx)
    → popa + iret
  → 回到 ring3 的 int $0x80 下一条指令（shell.tk 中: jmp 回主循环）
```

### 文件操作路径

```
ring0 "cat /etc/passwd"
  → shell_run → cmd_cat → tfs_read("/etc/passwd", buf, 256)
    → tfs_find("/etc/passwd") → slot=0
    → memcpy(buf, FS_BASE+files[0].offset, min(files[0].size, 255))
    → buf[n] = '\0' → cursor_write(buf) → 打印到屏幕

ring3 getch
  → mov $2,%eax; int $0x80
  → syscall_handler: r->eax = getch()
    → 阻塞等 keybuf 有数据
    → 返回 ASCII 码到 ring3 的 eax
```

---

## 六、文件依赖图

```
                     kernel.c
                    /   |    \
              auth.h  tfs.h  gdt.h → paging.h → idt.h
               |       |       |
            auth.c   tfs.c   gdt.c  paging.c  idt.c
               |       |       |       |        |
               +---+---+---+---+-------+--------+
                          |
                        isr.h
                      /   |   \
                 isr.c  proc.h shell.h
                         |       |
                      proc.c  shell.c
                         |
                      tklib.h
                         |
                   (tool) gen_tk.py

                  boot.S — 独立, 提供所有 ASM 存根
```

**关键依赖顺序（启动链不可乱）：**
```
gdt_install → tss_init → paging_init → page_map_user → idt_install → sti
    ↑ 先设段      ↑ 基于段    ↑ 之后才能开分页  ↑ 分页后才能开中断
```

---

## 七、常见坑记录

1. **QEMU 11 `-kernel` 废弃 Multiboot1** → 补了 Multiboot2 头 + PVH ELF Note。ELF 格式必须（flat binary 不支持）。
2. **GDT 数组溢出 3→6** → 最初只有 3 条目，加 ring3+TSS 后未扩容，黑屏。
3. **`lgdt` 传参** → 传的是 `gp` 结构体地址，不是 GDT 表地址。`gdt_flush((uint32_t)&gp)`。
4. **键盘 IRQ 号** → IRQ1 = IDT 索引 **33**（PIC 重映射后），不是 1。
5. **分页标志 `=` vs `|=`** → `page_dir[pd] = ...` 会覆盖 PRESENT，导致黑屏。
6. **BSS 未对齐** → `linker.ld` 缺 `ALIGN(4096)` 导致栈段不完整页覆盖。
7. **TSS `iopb` 字段序** → `iopb` 必须在偏移 0x64 位置，之前字段序颠倒导致 IOPB 无效。
8. **`tfs_get_size` break 位置** → `if(!name[j]) { if(...) {break;} }` → 两者同时结束时 break 被跳过 → 继续比较 `input` buffer 残留字符 → 误判 file not found。修正：break 提到 if 外面。
9. **SSE `#UD`** → ring3 代码用了 SSE 指令。编译加 `-mno-sse -mno-mmx` 解决。
10. **`tfs_read` 截断** → `buf[n]='\0'` 时 `n` 可能等于 `bufsize-1`，覆盖最后一个字节。`proc_load` 以 `size+1` 作为 bufsize 补偿。
