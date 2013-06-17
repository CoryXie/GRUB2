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


/*
 * Note: These functions defined in this file may be called from C.
 *       Be careful of that you must not modify some registers. Quote
 *       from gcc-2.95.2/gcc/config/i386/i386.h:

   1 for registers not available across function calls.
   These must include the FIXED_REGISTERS and also any
   registers that can be used without being saved.
   The latter must include the registers where values are returned
   and the register where structure-value addresses are passed.
   Aside from that, you can include as many other registers as you like.

  ax,dx,cx,bx,si,di,bp,sp,st,st1,st2,st3,st4,st5,st6,st7,arg
{  1, 1, 1, 0, 0, 0, 0, 1, 1,  1,  1,  1,  1,  1,  1,  1,  1 }
 */

/*
 * Note: GRUB is compiled with the options -mrtd and -mregparm=3.
 *       So the first three arguments are passed in %eax, %edx, and %ecx,
 *       respectively, and if a function has a fixed number of arguments
 *       and the number is greater than three, the function must return
 *       with "ret $N" where N is ((the number of arguments) - 3) * 4.
 */

#include <config.h>
#include <grub/symbol.h>
#include <multiboot.h>
#ifdef __APPLE__
#include <grub/i386/pc/memory.h>
#endif

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

	.file	"startup.S"

	.text

	.globl	start, _start, __start
start:
_start:
__start:
#ifdef __APPLE__
LOCAL(start):
#endif
	.code32

	movl	%ecx, (LOCAL(real_to_prot_addr) - _start) (%esi)
	movl	%edi, (LOCAL(prot_to_real_addr) - _start) (%esi)
	movl	%eax, (EXT_C(grub_realidt) - _start) (%esi)

	/* copy back the decompressed part (except the modules) */
#ifdef __APPLE__
	movl	$EXT_C(_edata), %ecx
	subl    $LOCAL(start), %ecx
#else
	movl	$(_edata - _start), %ecx
#endif
	movl	$(_start), %edi
	rep
	movsb

	movl	$LOCAL (cont), %esi
	jmp	*%esi
LOCAL(cont):

#if 0
	/* copy modules before cleaning out the bss */
	movl	EXT_C(grub_total_module_size), %ecx
	movl	EXT_C(grub_kernel_image_size), %esi
	addl	%ecx, %esi
	addl	$_start, %esi
	decl	%esi
	movl	$END_SYMBOL, %edi
	addl	%ecx, %edi
	decl	%edi
	std
	rep
	movsb
#endif

#ifdef __APPLE__
	/* clean out the bss */
	movl	$EXT_C(_edata), %edi

	/* compute the bss length */
	movl	$GRUB_MEMORY_MACHINE_SCRATCH_ADDR, %ecx
#else
	/* clean out the bss */
	movl	$BSS_START_SYMBOL, %edi

	/* compute the bss length */
	movl	$END_SYMBOL, %ecx
#endif
	subl	%edi, %ecx

	/* clean out */
	xorl	%eax, %eax
	cld
	rep
	stosb

	movl	%edx, EXT_C(grub_boot_device)

	/*
	 *  Call the start of main body of C code.
	 */
	call EXT_C(grub_main)

LOCAL(real_to_prot_addr):
	.long 0
LOCAL(prot_to_real_addr):
	.long 0

	.macro PROT_TO_REAL
	movl	LOCAL(prot_to_real_addr), %eax
	call	*%eax
	.endm

	.macro REAL_TO_PROT
	movl	LOCAL(real_to_prot_addr), %eax
	DATA32	call	*%ax
	.endm

/*
 * grub_exit()
 *
 * Exit the system.
 */
FUNCTION(grub_exit)
	PROT_TO_REAL
	.code16
	/* Tell the BIOS a boot failure. If this does not work, reboot.  */
	int	$0x18
	/* set 0x472 to 0x0000 for cold boot (0x1234 for warm boot) */
	xorw	%ax, %ax
	movw	$0x0472, %di
	movw	%ax, (%di)
	ljmp	$0xf000, $0xfff0
	.code32

/*
 * int grub_pxe_call (int func, void* data, grub_uint32_t pxe_rm_entry);
 */
FUNCTION(grub_pxe_call)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi
	pushl	%ebx

	movl	%ecx, %ebx
	movl	%eax, %ecx
	movl	%edx, %eax
	andl	$0xF, %eax
	shrl	$4, %edx
	shll	$16, %edx
	addl	%eax, %edx

	PROT_TO_REAL
	.code16

	pushl	%ebx
	pushl	%edx
	pushw	%cx
	movw	%sp, %bx
	lcall	*%ss:6(%bx)
	cld
	addw	$10, %sp
	movw	%ax, %cx

	REAL_TO_PROT
	.code32

	movzwl	%cx, %eax

	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp
	ret

#include "../int.S"

VARIABLE(grub_realidt)
	.long 0

#ifdef __APPLE__
	/* Older versions of objconv assume that there is the same number
	   of text and data sections. Hence this dummy.  */
	.section __TEXT, __zz_dummy
	.byte 0
	.globl EXT_C(_edata)
	.globl EXT_C(grub_boot_device)
	.zerofill __DATA, __aa_before_bss, EXT_C(_edata), 1, 0
	.zerofill __DATA, __bss, EXT_C(grub_boot_device), 4, 2
#else
	.bss
VARIABLE(grub_boot_device)
	.long	0
#endif
