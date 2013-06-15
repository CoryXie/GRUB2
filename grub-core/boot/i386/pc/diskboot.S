/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2006,2007,2009,2010   Free Software Foundation, Inc.
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

#include <grub/symbol.h>
#include <grub/machine/boot.h>

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
* @brief 启动代码第二阶段
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

/*
 *  defines for the code go here
 */

#define MSG(x)	movw $x, %si; call LOCAL(message)

	.file	"diskboot.S"

	.text

	/* Tell GAS to generate 16-bit instructions so that this code works
	   in real mode. */
	.code16

	.globl	start, _start
start:
_start:
	/*
	 * _start is loaded at 0x2000 and is jumped to with
	 * CS:IP 0:0x2000 in kernel.
	 */

	/*
	 * we continue to use the stack for boot.img and assume that
	 * some registers are set to correct values. See boot.S
	 * for more information.
	 */

	/* save drive reference first thing! */
	pushw	%dx

	/* print a notification message on the screen */
	pushw	%si
	MSG(notification_string)
	popw	%si

	/* this sets up for the first run through "bootloop" */
	movw	$LOCAL(firstlist), %di

	/* save the sector number of the second sector in %ebp */
	movl	(%di), %ebp

        /* this is the loop for reading the rest of the kernel in */
LOCAL(bootloop):

	/* check the number of sectors to read */
	cmpw	$0, 8(%di)

	/* if zero, go to the start function */
	je	LOCAL(bootit)

LOCAL(setup_sectors):
	/* check if we use LBA or CHS */
	cmpb	$0, -1(%si)

	/* use CHS if zero, LBA otherwise */
	je	LOCAL(chs_mode)

	/* load logical sector start */
	movl	(%di), %ebx
	movl	4(%di), %ecx

	/* the maximum is limited to 0x7f because of Phoenix EDD */
	xorl	%eax, %eax
	movb	$0x7f, %al

	/* how many do we really want to read? */
	cmpw	%ax, 8(%di)	/* compare against total number of sectors */

	/* which is greater? */
	jg	1f

	/* if less than, set to total */
	movw	8(%di), %ax

1:
	/* subtract from total */
	subw	%ax, 8(%di)

	/* add into logical sector start */
	addl	%eax, (%di)
	adcl	$0, 4(%di)

	/* set up disk address packet */

	/* the size and the reserved byte */
	movw	$0x0010, (%si)

	/* the number of sectors */
	movw	%ax, 2(%si)

	/* the absolute address */
	movl	%ebx, 8(%si)
	movl	%ecx, 12(%si)

	/* the segment of buffer address */
	movw	$GRUB_BOOT_MACHINE_BUFFER_SEG, 6(%si)

	/* save %ax from destruction! */
	pushw	%ax

	/* the offset of buffer address */
	movw	$0, 4(%si)

/*
 * BIOS call "INT 0x13 Function 0x42" to read sectors from disk into memory
 *	Call with	%ah = 0x42
 *			%dl = drive number
 *			%ds:%si = segment:offset of disk address packet
 *	Return:
 *			%al = 0x0 on success; err code on failure
 */

	movb	$0x42, %ah
	int	$0x13

	jc	LOCAL(read_error)

	movw	$GRUB_BOOT_MACHINE_BUFFER_SEG, %bx
	jmp	LOCAL(copy_buffer)

LOCAL(chs_mode):
	/* load logical sector start (top half) */
	movl	4(%di), %eax
	orl	%eax, %eax
	jnz	LOCAL(geometry_error)

	/* load logical sector start (bottom half) */
	movl	(%di), %eax

	/* zero %edx */
	xorl	%edx, %edx

	/* divide by number of sectors */
	divl	(%si)

	/* save sector start */
	movb	%dl, 10(%si)

	xorl	%edx, %edx	/* zero %edx */
	divl	4(%si)		/* divide by number of heads */

	/* save head start */
	movb	%dl, 11(%si)

	/* save cylinder start */
	movw	%ax, 12(%si)

	/* do we need too many cylinders? */
	cmpw	8(%si), %ax
	jge	LOCAL(geometry_error)

	/* determine the maximum sector length of this read */
	movw	(%si), %ax	/* get number of sectors per track/head */

	/* subtract sector start */
	subb	10(%si), %al

	/* how many do we really want to read? */
	cmpw	%ax, 8(%di)	/* compare against total number of sectors */


	/* which is greater? */
	jg	2f

	/* if less than, set to total */
	movw	8(%di), %ax

2:
	/* subtract from total */
	subw	%ax, 8(%di)

	/* add into logical sector start */
	addl	%eax, (%di)
	adcl	$0, 4(%di)

/*
 *  This is the loop for taking care of BIOS geometry translation (ugh!)
 */

	/* get high bits of cylinder */
	movb	13(%si), %dl

	shlb	$6, %dl		/* shift left by 6 bits */
	movb	10(%si), %cl	/* get sector */

	incb	%cl		/* normalize sector (sectors go
					from 1-N, not 0-(N-1) ) */
	orb	%dl, %cl	/* composite together */
	movb	12(%si), %ch	/* sector+hcyl in cl, cylinder in ch */

	/* restore %dx */
	popw	%dx
	pushw	%dx

	/* head number */
	movb	11(%si), %dh

	pushw	%ax	/* save %ax from destruction! */

/*
 * BIOS call "INT 0x13 Function 0x2" to read sectors from disk into memory
 *	Call with	%ah = 0x2
 *			%al = number of sectors
 *			%ch = cylinder
 *			%cl = sector (bits 6-7 are high bits of "cylinder")
 *			%dh = head
 *			%dl = drive (0x80 for hard disk, 0x0 for floppy disk)
 *			%es:%bx = segment:offset of buffer
 *	Return:
 *			%al = 0x0 on success; err code on failure
 */

	movw	$GRUB_BOOT_MACHINE_BUFFER_SEG, %bx
	movw	%bx, %es	/* load %es segment with disk buffer */

	xorw	%bx, %bx	/* %bx = 0, put it at 0 in the segment */
	movb	$0x2, %ah	/* function 2 */
	int	$0x13

	jc	LOCAL(read_error)

	/* save source segment */
	movw	%es, %bx

LOCAL(copy_buffer):

	/* load addresses for copy from disk buffer to destination */
	movw	10(%di), %es	/* load destination segment */

	/* restore %ax */
	popw	%ax

	/* determine the next possible destination address (presuming
		512 byte sectors!) */
	shlw	$5, %ax		/* shift %ax five bits to the left */
	addw	%ax, 10(%di)	/* add the corrected value to the destination
				   address for next time */

	/* save addressing regs */
	pusha
	pushw	%ds

	/* get the copy length */
	shlw	$3, %ax
	movw	%ax, %cx

	xorw	%di, %di	/* zero offset of destination addresses */
	xorw	%si, %si	/* zero offset of source addresses */
	movw	%bx, %ds	/* restore the source segment */

	cld		/* sets the copy direction to forward */

	/* perform copy */
	rep		/* sets a repeat */
	movsw		/* this runs the actual copy */

	/* restore addressing regs and print a dot with correct DS
	   (MSG modifies SI, which is saved, and unused AX and BX) */
	popw	%ds
	MSG(notification_step)
	popa

	/* check if finished with this dataset */
	cmpw	$0, 8(%di)
	jne	LOCAL(setup_sectors)

	/* update position to load from */
	subw	$GRUB_BOOT_MACHINE_LIST_SIZE, %di

	/* jump to bootloop */
	jmp	LOCAL(bootloop)

/* END OF MAIN LOOP */

LOCAL(bootit):
	/* print a newline */
	MSG(notification_done)
	popw	%dx	/* this makes sure %dl is our "boot" drive */
	ljmp	$0, $(GRUB_BOOT_MACHINE_KERNEL_ADDR + 0x200)


/*
 * BIOS Geometry translation error (past the end of the disk geometry!).
 */
LOCAL(geometry_error):
	MSG(geometry_error_string)
	jmp	LOCAL(general_error)

/*
 * Read error on the disk.
 */
LOCAL(read_error):
	MSG(read_error_string)

LOCAL(general_error):
	MSG(general_error_string)

/* go here when you need to stop the machine hard after an error condition */
LOCAL(stop):	jmp	LOCAL(stop)

notification_string:	.asciz "loading"

notification_step:	.asciz "."
notification_done:	.asciz "\r\n"

geometry_error_string:	.asciz "Geom"
read_error_string:	.asciz "Read"
general_error_string:	.asciz " Error"

/*
 * message: write the string pointed to by %si
 *
 *   WARNING: trashes %si, %ax, and %bx
 */

	/*
	 * Use BIOS "int 10H Function 0Eh" to write character in teletype mode
	 *	%ah = 0xe	%al = character
	 *	%bh = page	%bl = foreground color (graphics modes)
	 */
1:
	movw	$0x0001, %bx
	movb	$0xe, %ah
	int	$0x10		/* display a byte */

	incw	%si
LOCAL(message):
	movb	(%si), %al
	cmpb	$0, %al
	jne	1b	/* if not end of string, jmp to display */
	ret

/*
 *  This area is an empty space between the main body of code below which
 *  grows up (fixed after compilation, but between releases it may change
 *  in size easily), and the lists of sectors to read, which grows down
 *  from a fixed top location.
 */

	.word 0
	.word 0

	. = _start + 0x200 - GRUB_BOOT_MACHINE_LIST_SIZE
LOCAL(firstlist):	/* this label has to be before the first list entry!!! */
        /* fill the first data listing with the default */
blocklist_default_start:
	/* this is the sector start parameter, in logical sectors from
	   the start of the disk, sector 0 */
	.long 2, 0
blocklist_default_len:
	/* this is the number of sectors to read.  grub-mkimage
	   will fill this up */
	.word 0
blocklist_default_seg:
	/* this is the segment of the starting address to load the data into */
	.word (GRUB_BOOT_MACHINE_KERNEL_SEG + 0x20)
