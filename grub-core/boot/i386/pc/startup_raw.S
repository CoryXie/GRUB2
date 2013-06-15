/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2005,2006,2007,2008,2009,2011 Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <grub/symbol.h>
#include <grub/offsets.h>
#include <grub/machine/boot.h>
#include <grub/machine/memory.h>
#include <grub/machine/kernel.h>

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月8日
*
* @brief 启动代码第三阶段
*
* @note 注释详细内容:
*
* 第1步，X86 CPU启动时，CS=0xFFFF，IP=0，按照实模式工作，从地址0xFFFF0处取到第一条
* 指令；BIOS ROM的第一条指令就是放在这里的，因此开始了BIOS的运行。
* 
* 第2步，BIOS运行过程中，除了做基本的硬件检测和硬件初始化之外，还有一个关键的地方是要
* 设置实模式下的中端向量表【Interrupt Vector Table (IVT)】，位于0地址处，并安装对应中
* 断号的中断处理程序(ISR)。BIOS接着会检测系统的现有启动设备（用户可设置各个启动设备的
* 相对顺序），依次调用INT 0x19来读取其第一个扇区，并检测其启动标记【最末尾2个字节】是
* 否正确。 如果启动标记正确，就会把该启动扇区加载到内存地址0x07C00处，并且跳转到0x07C00
* 处接着执行。因此，在从实模式切换到保护模式之前，实际上都还是在BIOS的上下文中运行，
* 因此就可以做BIOS提供的中断调用，所使用的中断服务程序也就是前面在中断向量表中安装的
* ISR。
* 
* 第3步，BIOS加载启动扇区并跳转到0x07C00处后，就开始了真正的Bootloader的执行，这就是我
* 们这里要分析的GRUB的第一段真正被CPU执行的代码。这段代码位于
* 【grub-2.00\grub-core\boot\i386\pc\boot.S】。BIOS跳转到0x07C00是通过JMP实现的 
* (CS:IP 0:0x7C00)，因此，当在boot.S中开始运行时的代码段寄存器和指令指针寄存分别是
* CS=0，IP=0x7C00。
* 
* 第4步，启动扇区程序目前还处于实模式，开始设置数据段和堆栈段寄存器（DS和SS）为0，而
* 代码段寄存器CS在前面一步已经被设置为0。接着设置堆栈指针寄存器SP为
* GRUB_BOOT_MACHINE_STACK_SEG，即为0x2000。
* 
* 第5步，检测启动设备（设备号由BIOS在跳转前保存在DL寄存器中）是否支持LBA模式。并根据
* 检测结果决定按照LBA模式还是CHS模式访问启动设备。
* 
* 第6步，根据前面检测的启动设备访问模式，选择从lba_mode分支还是从chs_mode分支读取GRUB
* 下一阶段的代码（即diskboot.S对应代码）到内存中，而这段代码的起始扇区被标记为
* kernel_sector，扇区个数为1。kernel_sector标记所在地址的值并不是在编译时固定的，而
* 是在编译完成后，通过grub-mkimage工具填入的（修改boot.S对应的二进制代码）。而
* diskboot.S对应的代码被读入的内存，该内存段GRUB_BOOT_MACHINE_BUFFER_SEG的值为
* 0x7000，也即diskboot.S对应代码会被读到地址0x70000处。
* 
* 第7步，当diskboot.S对应代码被读入后，还会被通过copy_buffer拷贝到
* GRUB_BOOT_MACHINE_KERNEL_ADDR（即GRUB_BOOT_MACHINE_KERNEL_SEG对应地址），
* 而这个GRUB_BOOT_MACHINE_KERNEL_SEG是在编译时通过拼接而成的，来自于自动生成的位于
* include/grub/offset.h中的GRUB_BOOT_I386_PC_KERNEL_SEG，其值为0x800，因此diskboot.S
* 对应代码被拷贝到0x8000处。 
* 
* 第8步，通过jmp *(kernel_address)跳转到0x8000处继续执行diskboot.S对应代码。
* 
* 第9步，在diskboot.S对应代码内部，通过bootloop读入GRUB第三阶段代码。这里的第三阶段
* 代码来自blocklist_default_start标号处的内容对应的起始扇区， 默认从sector 2开始，
* 总共的扇区数是blocklist_default_len，而这些值是grub-mkimage在编译后期通过
* 改写diskboot.S对应的二进制文件而填入的。而这段代码被读入的目的地址是由
* blocklist_default_seg标号处的内容，即(GRUB_BOOT_MACHINE_KERNEL_SEG + 0x20)
* 所决定，而GRUB_BOOT_MACHINE_KERNEL_SEG如前所述其值为0x800，因此
* blocklist_default_seg标号处的内容为0x820，也就是说，GRUB的第三阶段代码被读入到了
* 0x8200处。
* 
* 第10步，当diskboot.S中的bootloop读取完毕所有的GRUB第三阶段代码时，就跳转到
* bootit处执行。进而又jmp到GRUB_BOOT_MACHINE_KERNEL_ADDR+0x200（即0x8200）处，
* 开始执行刚刚读进来的GRUB第三阶段代码处。
* 
* 第11步，第三阶段代码是经过压缩的，但是映像开头是grub-core/boot/i386/pc/startup_raw.S和
* grub-core/boot/i386/pc/lzma_decode.S的一段未压缩代码，以便进入保护模式并用以解压缩
* 至GRUB_MEMORY_MACHINE_DECOMPRESSION_ADDR，也即0x100000处,并跳转到该地址继续执行，
* 而这部分代码的开始就是grub-core/kern/i386/pc/startup.S。
* 
* 第12步，这段代码的最开始部分就是将刚才解压缩的代码复制回到其链接地址
* （GRUB_KERNEL_I386_PC_LINK_ADDR，即0x9000），然后又跳转回到链接地址，清零bss段，
* 最终调用grub-core/kern/main.c中的grub_main()。于是GRUB开始进入C语言编写的kernel
* （GRUB自己的内核）执行。
**/

#define ABS(x)	((x) - LOCAL (base) + GRUB_BOOT_MACHINE_KERNEL_ADDR + 0x200)

	.file	"startup_raw.S"

	.text

	/* Tell GAS to generate 16-bit instructions so that this code works
	   in real mode. */
	.code16

	.globl	start, _start
start:
_start:
LOCAL (base):
	/*
	 *  Guarantee that "main" is loaded at 0x0:0x8200.
	 */
#ifdef __APPLE__
	ljmp $0, $(ABS(LOCAL (codestart)) - 0x10000)
#else
	ljmp $0, $ABS(LOCAL (codestart))
#endif

	/*
	 *  This is a special data area.
	 */

	. = _start + GRUB_DECOMPRESSOR_MACHINE_COMPRESSED_SIZE
LOCAL(compressed_size):
	.long 0
	. = _start + GRUB_DECOMPRESSOR_MACHINE_UNCOMPRESSED_SIZE
LOCAL(uncompressed_size):
	.long 0

	. = _start + GRUB_KERNEL_I386_PC_REED_SOLOMON_REDUNDANCY
reed_solomon_redundancy:
	.long	0
	. = _start + GRUB_KERNEL_I386_PC_NO_REED_SOLOMON_LENGTH
	.short	(LOCAL(reed_solomon_part) - _start)

/*
 *  This is the area for all of the special variables.
 */
	. = _start + GRUB_DECOMPRESSOR_I386_PC_BOOT_DEVICE
LOCAL(boot_dev):
	.byte	0xFF, 0xFF, 0xFF
LOCAL(boot_drive):
	.byte	0x00

/* the real mode code continues... */
LOCAL (codestart):
	cli		/* we're not safe here! */

	/* set up %ds, %ss, and %es */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %ss
	movw	%ax, %es

	/* set up the real mode/BIOS stack */
	movl	$GRUB_MEMORY_MACHINE_REAL_STACK, %ebp
	movl	%ebp, %esp

	sti		/* we're safe again */

	/* save the boot drive */
	ADDR32	movb	%dl, LOCAL(boot_drive)

	/* reset disk system (%ah = 0) */
	int	$0x13

	/* transition to protected mode */
	DATA32	call real_to_prot

	/* The ".code32" directive takes GAS out of 16-bit mode. */
	.code32

	incl	%eax
	cld
	call	grub_gate_a20

	movl	LOCAL(compressed_size), %edx
#ifdef __APPLE__
	addl    $decompressor_end, %edx
	subl    $(LOCAL(reed_solomon_part)), %edx
#else
	addl    $(LOCAL(decompressor_end) - LOCAL(reed_solomon_part)), %edx
#endif
	movl    reed_solomon_redundancy, %ecx
	leal    LOCAL(reed_solomon_part), %eax
	cld
	call    EXT_C (grub_reed_solomon_recover)
	jmp	post_reed_solomon

#include "../../../kern/i386/realmode.S"

#include <rs_decoder.S>

	.text

/*
 * grub_gate_a20(int on)
 *
 * Gate address-line 20 for high memory.
 *
 * This routine is probably overconservative in what it does, but so what?
 *
 * It also eats any keystrokes in the keyboard buffer.  :-(
 */

grub_gate_a20:	
	movl	%eax, %edx

gate_a20_test_current_state:
	/* first of all, test if already in a good state */
	call	gate_a20_check_state
	cmpb	%al, %dl
	jnz	gate_a20_try_bios
	ret

gate_a20_try_bios:
	/* second, try a BIOS call */
	pushl	%ebp
	call	prot_to_real

	.code16
	movw	$0x2400, %ax
	testb	%dl, %dl
	jz	1f
	incw	%ax
1:	int	$0x15

	DATA32	call	real_to_prot
	.code32

	popl	%ebp
	call	gate_a20_check_state
	cmpb	%al, %dl
	jnz	gate_a20_try_system_control_port_a
	ret

gate_a20_try_system_control_port_a:
	/*
	 * In macbook, the keyboard test would hang the machine, so we move
	 * this forward.
	 */
	/* fourth, try the system control port A */
	inb	$0x92
	andb	$(~0x03), %al
	testb	%dl, %dl
	jz	6f
	orb	$0x02, %al
6:	outb	$0x92

	/* When turning off Gate A20, do not check the state strictly,
	   because a failure is not fatal usually, and Gate A20 is always
	   on some modern machines.  */
	testb	%dl, %dl
	jz	7f
	call	gate_a20_check_state
	cmpb	%al, %dl
	jnz	gate_a20_try_keyboard_controller
7:	ret

gate_a20_flush_keyboard_buffer:
	inb	$0x64
	andb	$0x02, %al
	jnz	gate_a20_flush_keyboard_buffer
2:
	inb	$0x64
	andb	$0x01, %al
	jz	3f
	inb	$0x60
	jmp	2b
3:
	ret

gate_a20_try_keyboard_controller:
	/* third, try the keyboard controller */
	call    gate_a20_flush_keyboard_buffer

	movb	$0xd1, %al
	outb	$0x64
4:
	inb	$0x64
	andb	$0x02, %al
	jnz	4b

	movb	$0xdd, %al
	testb	%dl, %dl
	jz	5f
	orb	$0x02, %al
5:	outb	$0x60
	call    gate_a20_flush_keyboard_buffer

	/* output a dummy command (USB keyboard hack) */
	movb	$0xff, %al
	outb	$0x64
	call    gate_a20_flush_keyboard_buffer

	call	gate_a20_check_state
	cmpb	%al, %dl
	/* everything failed, so restart from the beginning */
	jnz	gate_a20_try_bios
	ret

gate_a20_check_state:
	/* iterate the checking for a while */
	movl	$100, %ecx
1:
	call	3f
	cmpb	%al, %dl
	jz	2f
	loop	1b
2:
	ret
3:
	pushl	%ebx
	pushl	%ecx
	xorl	%eax, %eax
	/* compare the byte at 0x8000 with that at 0x108000 */
	movl	$GRUB_BOOT_MACHINE_KERNEL_ADDR, %ebx
	pushl	%ebx
	/* save the original byte in CL */
	movb	(%ebx), %cl
	/* store the value at 0x108000 in AL */
	addl	$0x100000, %ebx
	movb	(%ebx), %al
	/* try to set one less value at 0x8000 */
	popl	%ebx
	movb	%al, %ch
	decb	%ch
	movb	%ch, (%ebx)
	/* serialize */
	outb	%al, $0x80
	outb	%al, $0x80
	/* obtain the value at 0x108000 in CH */
	pushl	%ebx
	addl	$0x100000, %ebx
	movb	(%ebx), %ch
	/* this result is 1 if A20 is on or 0 if it is off */
	subb	%ch, %al
	xorb	$1, %al
	/* restore the original */
	popl	%ebx
	movb	%cl, (%ebx)
	popl	%ecx
	popl	%ebx
	ret

LOCAL(reed_solomon_part):

/*
 * Support for booting GRUB from a Multiboot boot loader (e.g. GRUB itself).
 * This uses the a.out kludge to load raw binary to the area starting at 1MB,
 * and relocates itself after loaded.
 */
	.p2align	2	/* force 4-byte alignment */
multiboot_header:
	/* magic */
	.long	0x1BADB002
	/* flags */
	.long	(1 << 16)
	/* checksum */
	.long	-0x1BADB002 - (1 << 16)
	/* header addr */
	.long	multiboot_header - _start + 0x100000 + 0x200
	/* load addr */
	.long	0x100000
	/* load end addr */
	.long	0
	/* bss end addr */
	.long	0
	/* entry addr */
	.long	multiboot_entry - _start + 0x100000 + 0x200

multiboot_entry:
	.code32
	/* obtain the boot device */
	movl	12(%ebx), %edx

	movl	$GRUB_MEMORY_MACHINE_PROT_STACK, %ebp
	movl	%ebp, %esp

	/* relocate the code */
#ifdef __APPLE__
	LOCAL(compressed_size_offset) = LOCAL(compressed_size) - LOCAL(base)
	movl	$0x200, %ecx
	addl    $decompressor_end, %ecx
	subl    $LOCAL(base), %ecx
	addl	LOCAL(compressed_size_offset) + 0x100000 + 0x200, %ecx
#else
	movl	$(LOCAL(decompressor_end) - _start + 0x200), %ecx
	addl	LOCAL(compressed_size) - _start + 0x100000 + 0x200, %ecx
#endif
	movl	$0x100000, %esi
	movl	$GRUB_BOOT_MACHINE_KERNEL_ADDR, %edi
	cld
	rep
	movsb
	/* jump to the real address */
	movl	$multiboot_trampoline, %eax
	jmp	*%eax

multiboot_trampoline:
	/* fill the boot information */
	movl	%edx, LOCAL(boot_dev)
	shrl	$24, %edx
	/* enter the usual booting */
	call	prot_to_real
	.code16
	jmp     LOCAL (codestart)
	.code32

post_reed_solomon:

#ifdef ENABLE_LZMA
	movl	$GRUB_MEMORY_MACHINE_DECOMPRESSION_ADDR, %edi
#ifdef __APPLE__
	movl	$decompressor_end, %esi
#else
	movl	$LOCAL(decompressor_end), %esi
#endif
	pushl	%edi
	movl	LOCAL (uncompressed_size), %ecx
	leal	(%edi, %ecx), %ebx
	/* Don't remove this push: it's an argument.  */
	push 	%ecx
	call	_LzmaDecodeA
	pop	%ecx
	/* _LzmaDecodeA clears DF, so no need to run cld */
	popl	%esi
#endif

	movl	LOCAL(boot_dev), %edx
	movl	$prot_to_real, %edi
	movl	$real_to_prot, %ecx
	movl	$LOCAL(realidt), %eax
	jmp	*%esi

#ifdef ENABLE_LZMA
#include "lzma_decode.S"
#endif

	.p2align 4

#ifdef __APPLE__
	.zerofill __DATA, __aa_before_bss, decompressor_end, 10, 0
#else
	.bss
LOCAL(decompressor_end):
#endif
