```
  ::::::::::: ::::::::  :::    ::: ::::::::::: ::::::::   :::::::: 
     :+:    :+:    :+: :+:   :+:      :+:    :+:    :+: :+:    :+: 
    +:+    +:+    +:+ +:+  +:+       +:+    +:+    +:+ +:+         
   +#+    +#+    +:+ +#++:++        +#+    +#+    +:+ +#++:++#++   
  +#+    +#+    +#+ +#+  +#+       +#+    +#+    +#+        +#+    
 #+#    #+#    #+# #+#   #+#      #+#    #+#    #+# #+#    #+#     
###     ########  ###    ### ########### ########   ########       
```
> 一个从零手搓的 x86 内核。小到塞进 L1 缓存，干净到能教你妈用。
> 蓝色底色，黄色高亮，白色光标，打字机美学。

---

## 宣言

**操作系统没那么可怕。**

这堆东西说到底就是几个概念：中断、分页、系统调用。每个都能用一页纸讲清楚。
一个电子信息的大一学生，啃两周概念，也能让 CPU 乖乖听自己的话。

这里没有魔法。只有 C、汇���、和一份`intel-sdm.pdf`。

## 这是什么

TokiOS — 纯 C + AT&T 汇编，从 bootloader 到 shell，全部手写。零依赖。零借口。

```
TokiOS> cout hello world
hello world

TokiOS> where
row=3, col=0
```

**Shell 设计原则**：新手敲第一个命令就能看懂。想深挖的人，每个命令下面都是完整的内核机制。
自然语言动词，无脑入门，天花板在文件系统驱动之后。

## 构建

一个 32 位 x86 工具链。四行，一把梭。

```bash
as --32 boot.S -o boot.o
gcc -m32 -c *.c -ffreestanding -nostdlib -fno-pie -fno-stack-protector
ld -m elf_i386 -T linker.ld *.o -o tokios.bin
```

没有 cmake。没有 autotools。没有 `package.json`。

## 运行

```bash
qemu-system-i386 -machine pc -kernel tokios.bin
```

QEMU 7 到 11。Multiboot、PVH，你来哪个它接哪个。

## 解剖

```
boot.S      一切的起点。Multiboot 头、GDT、IDT、IRQ 存根。
kernel.c    一个函数：清屏、打 banner、交棒给 shell。
gdt.c/h     平坦内存模型。Ring 0。不设防。
idt.c/h     中断向量表。PIC 重映射。异常？优雅地忽略。
isr.c/h     中断分派。只给键盘留了门。
keyboard.c  扫描码→ASCII。128 项查表。回显。缓冲。光标。
shell.c/h   strncmp + atoi + write_int + 命令分发。一个文件。
linker.ld   ELF 布局。Multiboot 头在最前面，不然 QEMU 不搭理你。
```

## 命令

| 动词 | 简写 | 效果 |
|------|------|------|
| `cout <text>` | | 回显参数 |
| `clear` | `cls` | 清屏 |
| `where` | `w` | 光标在哪 |
| `go <row>` | | 跳到指定行 |
| `up [n]` | | 上移 |
| `down [n]` | | 下移 |
| `home` | | 归零 |
| `help` | `?` | 列出命令 |
| `shutdown` | | 停机 |

`show` `to` `new` `del` `copy` `move` — 文件系统占位。磁盘驱动还没写，你来早了。

## 教条

- **一个文件一个概念。** 不是一个函数一个文件。
- **零动态分配。** 要堆？自己写。
- **动词别超过两个音节。** 否则改名。
- **启动信息别啰嗦。** TokiOS 几个字够用了。
- **代码即文档。** ASCII 艺术是附赠的。

## 许可

MIT。拿走、拆开、学。重写成 Rust 也行。
