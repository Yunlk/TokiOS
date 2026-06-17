<div align="center">
<pre>
  ::::::::::: ::::::::  :::    ::: ::::::::::: ::::::::   ::::::::
     :+:    :+:    :+: :+:   :+:      :+:    :+:    :+: :+:    :+:
    +:+    +:+    +:+ +:+  +:+       +:+    +:+    +:+ +:+
   +#+    +#+    +:+ +#++:++        +#+    +#+    +:+ +#++:++#++
  +#+    +#+    +#+ +#+  +#+       +#+    +#+    +#+        +#+
 #+#    #+#    #+# #+#   #+#      #+#    #+#    #+# #+#    #+#
###     ########  ###    ### ########### ########   ########
</pre>
</div>

---

一个 x86 内核。大一学生写的。跑在 QEMU 上，还没上真机。

MVP 目标：最小内核（~10k 行 C）+ 丰富的中文 Shell。

> 📐 架构详解见 **[ARCHITECTURE.md](ARCHITECTURE.md)** — 文件职责、调用链、数据流、常见坑。

## 当前状态

分页开着。Ring 3 用户态跑 `.tk` 程序。`int 0x80` 系统调用 13 个。TokiFS v0.2 — 16 槽，1MB RAM 盘，删除紧凑回收。14 个 shell 命令。多用户认证（SHA-256 + `/etc/passwd`，最多 8 用户，输错 3 次冻结 30 秒）。TSS IOPB 已设，syscall 号边界已检查。

```
TokiOS booted.
[auth] new system -- creating tokisora (pass: tokisora)

TokiOS> login tokisora tokisora
login ok -- hello, tokisora

TokiOS> run shell.tk
TokiOS Shell> ls
  shell.tk
TokiOS Shell> exit

TokiOS>
```

内核 12 个 C 文件、8 个头文件、1 个汇编、1 个链接脚本，约 1100 行 C + 220 行 ASM。

工具：`tools/gen_tk.py`（234 行），两遍汇编器，rel8 跳转回填、绝对地址嵌入、字符串缓冲，生成 `kernel.c` 里的 `shell_code[]`。

## 命令

| 命令 | 效果 |
|------|------|
| `ls` | 列出 TFS 文件 |
| `run <file>` | 加载并运行 `.tk`（ring 3） |
| `cat <file>` | 打印文件内容 |
| `touch <file>` | 新建空文件 |
| `rm <file>` | 删除文件 |
| `clear` | 清屏 |
| `info` | CR0 / CR2 / CR3 |
| `help` | 列出命令 |
| `login <user> <pass>` | 登录 |
| `logout` | 注销 |
| `whoami` | 当前用户 |
| `passwd <new>` | 改密码 |
| `useradd <u> <p>` | 新建用户（最多 8 个） |
| `crash` | 触发 #DE 测试异常处理器 |

## 系统调用

`int 0x80` trap gate（0xEF），eax 分发，寄存器传参，SYS_MAX=13 边界检查。

| # | 名 | ebx | ecx | 返回 eax |
|---|------|-----|-----|----------|
| 1 | `COUT` | 字符串地址 | — | — |
| 2 | `GETCH` | — | — | 字符 |
| 3 | `CUR_SET` | 位置 | — | — |
| 4 | `CUR_GET` | — | — | 光标位置 |
| 5 | `TFS_LIST` | — | — | — |
| 6 | `TFS_READ` | 文件名 | 缓冲区 | 读取字节数 |
| 7 | `TFS_CREATE` | 文件名 | 数据 | 0=ok |
| 8 | `TFS_DELETE` | 文件名 | — | 0=ok |
| 9 | `EXIT` | — | — | 不返回 |
| 10 | `HEX` | 值 | — | — |
| 11 | `BACKSPACE` | — | — | — |
| 12 | `LOGIN` | 用户名 | 密码 | 1=ok |
| 13 | `EXEC` | 文件名 | — | 不返回 / 0=失败 |

## 内存布局

```
0x000000    IVT / BDA
0x100000    页目录
0x101000    页表（恒等映射 0–4MB）
0x200000    用户代码页
0x300000    TokiFS 数据区（1MB）
0xB8000     VGA 文本缓冲
```

## 设计共识

### 进程模型

- `run myapp.tk` 一步启动，不学 fork+exec
- 进程即文件：删除 `.tk` = 进程结束，无僵尸/匿名进程

### .tk 文件格式

```
toki(4B) | 入口地址(4B) | 代码大小(4B) | flags(4B) | 机器码
```

`tk_validate()` 检查魔数 `0x696B6F74`、code_size 在 1 字节到 1MB 间、flags=0。

### 系统调用

当前用 `int 0x80` trap gate（临时方案）。规划：函数钩子跳转表，同 ring 约 3 周期开销。

### 权限模型

| Ring | 谁 | 隔离 |
|------|-----|------|
| 0 | 内核 | GDT DPL=0 |
| 3 | 所有用户 / .tk 进程 | GDT DPL=3 + 页表 U/S + TSS IOPB |

**认证**：`login` 命令输入密码 → SHA-256 → 比对 `/etc/passwd` 中的 hex 哈希。  
**防暴力**：任意用户输错 3 次 → 30 秒 busy-wait 锁死 → 计数器归零。  
**存储**：`/etc/passwd` 格式 `user:sha256hex\n`，最多 8 用户，改密/新建用户即时 sync 回 TFS。  
**默认账户**：首次启动自动创建 `tokisora`（密码同用户名）。

### 密钥对方案

公钥认证留到网络栈阶段再做（Ed25519 验证器 ~200 行，远超当前规模）。

### 共享内存（已规划）

- `tk_share` 钩子握手，显式声明
- 跨 ring 共享需签名+审计
- 继承单向往上：子死→父，父死→祖

### 崩溃模型

进程崩溃 → dump 寄存器 → kill 进程 → reboot 进程（非全系统）

### 网络栈（已规划）

自写最小栈（ARP + IP + TCP），不依赖 lwIP。

## 进度

**2026-06-17**
多用户认证框架：`auth.c/h`（249 行），SHA-256 密码哈希，`/etc/passwd`（`user:hex`，最多 8 用户）。`login`/`logout`/`whoami`/`passwd`/`useradd` 全部 5 个命令。输错 3 次冻结 30 秒。默认账户 `tokisora`。syscall 边界检查（`SYS_MAX=13`）。TokiFS v0.2：`tfs_find()` 统一名字查找，`tfs_delete` 数据前移紧凑回收。`SYS_EXEC`（#13）：ring3 进程加载新 .tk 替换自身。修复 `tfs_get_size` 名字比较 break 位置 bug（input buffer 残留误判 file not found）。VGA 滚屏（`scroll_up()`）+ UTF-8 乱码修复。`ARCHITECTURE.md` 架构文档。boot 消息换行 + 光标同步。README 全面更新。

**2026-06-16**
`shell.tk` 完善：退格修复（`SYS_BACKSPACE` 内核侧直接 VGA 擦除，不依赖 cursor_write `\b`）、Enter 换行修正、命令解析回显 bug（ebx 覆盖回路）修复。TSS IOPB 完工 — `iopb ≥ sizeof(tss_t)` 令 ring3 任何 IO 触发 #GP。CPU 异常 0–31 处理器（dump 寄存器后 halt）。`.tk` 加载器完整：16 字节头验证 + ring3 跳转。`gen_tk.py` 两遍汇编器。Ring3 全链路跑通。

**2026-06-15**
Ring 3 用户态打通。`proc_load` → `page_map_user` → `proc_start(iret)`。`int 0x80` trap gate + 9 个系统调用。TokiFS v0.1（线性分配）。修了键盘 IRQ ring3 泄露、`info` CR0 打印溢出、二进制嵌入 off-by-one 等 bug。

**2026-06-14**
分页启用（恒等映射 0–4MB + 用户 2MB–3MB）。GDT 6 条目（ring0 + ring3 + TSS）。Shell 命令分发 if-else→函数指针表。键盘环形缓冲区（32 字节）。

**2026-06-13**
Multiboot → GDT → IDT → PIC 重映射 → 键盘中断 → scancode→ASCII → VGA 回显 → 光标 → Shell。和 QEMU 11 斗了一整天：Multiboot1 `-kernel` 被废，补 Multiboot2 头不行，最后加 PVH ELF Note 才过。

## 构建

```bash
as --32 boot.S -o boot.o
gcc -m32 -c *.c -ffreestanding -nostdlib -fno-pie -fno-stack-protector -mno-sse -mno-mmx
ld -m elf_i386 -T linker.ld *.o -o tokios.elf
```

QEMU 11 必须用 ELF 格式（`-kernel tokios.elf`），flat binary 不支持。

## 文件

### 内核

| 文件 | 行 | 职责 |
|------|-----|------|
| `boot.S` | 218 | Multiboot/PVH 头，GDT/IDT flush，IRQ/syscall 存根，`proc_start` |
| `kernel.c` | 73 | 入口：初始化链 + 嵌入 shell.tk + HLT 循环 |
| `gdt.c/h` | 37 | 6 条目 GDT（null/ring0 code/ring0 data/ring3 code/ring3 data/TSS） |
| `idt.c/h` | 87 | 256 条目 IDT，PIC 重映射，`int 0x80` trap gate（DPL=3） |
| `isr.c/h` | 35 | 中断分派，`struct regs`，键盘/光标 API |
| `keyboard.c` | 138 | scancode→ASCII 查表，VGA 回显 + 滚屏，环形缓冲，ring0/3 隔离 |
| `paging.c/h` | 23 | 恒等映射 0–4MB，`page_map_user()` |
| `proc.c/h` | 30 | `.tk` 加载器：TFS 读取 → 验证 → 映射用户页 → iret ring3 |
| `shell.c/h` | 223 | ring0 命令行：14 命令 + 函数指针表分发 + 参数解析 |
| `syscall.c/h` | 79 | `int 0x80` 分发（SYS_COUT=1 ~ SYS_EXEC=13），SYS_MAX 边界 |
| `tfs.c/h` | 102 | TokiFS v0.2：16 槽，1MB RAM 盘，`tfs_find` 统一查找，删除紧凑回收 |
| `auth.c/h` | 249 | 多用户认证：SHA-256 哈希，`/etc/passwd`，3 次冻结，最多 8 用户 |
| `tklib.h` | — | `.tk` 文件格式定义（16 字节头 + `tk_validate`） |
| `linker.ld` | — | 链接脚本（1MB 起点，BSS 4KB 对齐） |

### 工具 & demo

| 文件 | 行 | 职责 |
|------|-----|------|
| `tools/gen_tk.py` | 234 | 两遍汇编器：rel8 回填 + 绝对地址嵌入 → `shell_code[]` |
| `hello_tk.c` | 56 | ring3 demo 程序（cout + getch + exit） |

## 已知攻击面

| 问题 | 缓解 | 状态 |
|------|------|------|
| ring3 栈无守护页 | 分页后加守护页 | TODO |
| TFS 槽位越界写 | syscall 边界 + 槽位检查 | TODO |
| 双重异常→triple fault | 异常时关中断 | TODO |
| 键盘缓冲满丢字符 | 环形缓冲 back-pressure | TODO |
| syscall 号边界 | `if (r->eax < 1 \|\| r->eax > SYS_MAX) return` | ✅ |
| TFS 删除不回收 | 数据前移紧凑（v0.2） | ✅ |
| ring3 直接 IO | TSS IOPB ≥ sizeof(tss_t) | ✅ |
| 密码明文 | SHA-256 哈希 | ✅ |
| 暴力破解 | 3 次 → 30s 冻结 | ✅ |

## 与之相近的 OS

**Exokernel（MIT, 1995）** — 最像。内核不抽象硬件，只做安全多路复用。应用拿函数指针直接操作资源。TokiOS 的「内核导出 hook、.tk 声明依赖」就是这套。

**Plan 9（Bell Labs）** — 精神祖先。「一切皆文件」推到极致。`/proc` 里每个运行中的进程就是一个文件。TokiOS 的「进程 = .tk 文件，删除即杀死」直接从这里来。

**xv6（MIT）** — 同是教学内核，方向相反。xv6 教你怎么做小型 Unix：fork+exec、管道、fd、POSIX。TokiOS 教你怎么不学 Unix：没有 fork，没有管道，一切是文件名不是 fd。

**TempleOS** — 同是一个人写的 x86 内核。Terry Davis 所有代码 ring 0。TokiOS 有 ring 分层、进程隔离、密码提权，方向相反。

**明确拒绝的**
- Unix/POSIX — 没有 fork，没有 exec（指 `execve`），没有 fd
- Linux — 不要 3000 万行，不要设备树
- Windows — 不是闭源

## 许可

MIT。拿去拆，别署名。
