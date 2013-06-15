/* -*-Asm-*- */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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
* @brief 启动代码第一阶段
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

	/* Print message string */
#define MSG(x)	movw $x, %si; call LOCAL(message)
#define ERR(x)	movw $x, %si; jmp LOCAL(error_message)

	.file	"boot.S"

	.text

	/* Tell GAS to generate 16-bit instructions so that this code works
	   in real mode. */
	.code16

.globl _start, start;
_start:
start:
	/*
	 * _start is loaded at 0x7c00 and is jumped to with CS:IP 0:0x7c00
	 */

	/*
	 * Beginning of the sector is compatible with the FAT/HPFS BIOS
	 * parameter block.
	 */

	jmp	LOCAL(after_BPB)
	nop	/* do I care about this ??? */

	/*
	 * This space is for the BIOS parameter block!!!!  Don't change
	 * the first jump, nor start the code anywhere but right after
	 * this area.
	 */

	. = _start + GRUB_BOOT_MACHINE_BPB_START
	. = _start + 4

	/* scratch space */
mode:
	.byte	0
disk_address_packet:
sectors:
	.long	0
heads:
	.long	0
cylinders:
	.word	0
sector_start:
	.byte	0
head_start:
	.byte	0
cylinder_start:
	.word	0
	/* more space... */

	. = _start + GRUB_BOOT_MACHINE_BPB_END

	/*
	 * End of BIOS parameter block.
	 */

kernel_address:
	.word	GRUB_BOOT_MACHINE_KERNEL_ADDR

	. = _start + GRUB_BOOT_MACHINE_KERNEL_SECTOR
kernel_sector:
	.long	1, 0

	. = _start + GRUB_BOOT_MACHINE_BOOT_DRIVE
boot_drive:
	.byte 0xff	/* the disk to load kernel from */
			/* 0xff means use the boot drive */

LOCAL(after_BPB):

/* general setup */
	cli		/* we're not safe here! */

        /*
         * This is a workaround for buggy BIOSes which don't pass boot
         * drive correctly. If GRUB is installed into a HDD, check if
         * DL is masked correctly. If not, assume that the BIOS passed
         * a bogus value and set DL to 0x80, since this is the only
         * possible boot drive. If GRUB is installed into a floppy,
         * this does nothing (only jump).
         */
	. = _start + GRUB_BOOT_MACHINE_DRIVE_CHECK
boot_drive_check:
        jmp     3f	/* grub-setup may overwrite this jump */
        testb   $0x80, %dl
        jz      2f
3:
	/* Ignore %dl different from 0-0x0f and 0x80-0x8f.  */
	testb   $0x70, %dl
	jz      1f
2:	
        movb    $0x80, %dl
1:
	/*
	 * ljmp to the next instruction because some bogus BIOSes
	 * jump to 07C0:0000 instead of 0000:7C00.
	 */
	ljmp	$0, $real_start

real_start:

	/* set up %ds and %ss as offset from 0 */
	xorw	%ax, %ax
	movw	%ax, %ds
	movw	%ax, %ss

	/* set up the REAL stack */
	movw	$GRUB_BOOT_MACHINE_STACK_SEG, %sp

	sti		/* we're safe again */

	/*
	 *  Check if we have a forced disk reference here
	 */
	movb   boot_drive, %al
	cmpb	$0xff, %al
	je	1f
	movb	%al, %dl
1:
	/* save drive reference first thing! */
	pushw	%dx

	/* print a notification message on the screen */
	MSG(notification_string)

	/* set %si to the disk address packet */
	movw	$disk_address_packet, %si

	/* check if LBA is supported */
	movb	$0x41, %ah
	movw	$0x55aa, %bx
	int	$0x13

	/*
	 *  %dl may have been clobbered by INT 13, AH=41H.
	 *  This happens, for example, with AST BIOS 1.04.
	 */
	popw	%dx
	pushw	%dx

	/* use CHS if fails */
	jc	LOCAL(chs_mode)
	cmpw	$0xaa55, %bx
	jne	LOCAL(chs_mode)

	andw	$1, %cx
	jz	LOCAL(chs_mode)

lba_mode:
	xorw	%ax, %ax
	movw	%ax, 4(%si)

	incw	%ax
	/* set the mode to non-zero */
	movb	%al, -1(%si)

	/* the blocks */
	movw	%ax, 2(%si)

	/* the size and the reserved byte */
	movw	$0x0010, (%si)

	/* the absolute address */
	movl	kernel_sector, %ebx
	movl	%ebx, 8(%si)
	movl	kernel_sector + 4, %ebx
	movl	%ebx, 12(%si)

	/* the segment of buffer address */
	movw	$GRUB_BOOT_MACHINE_BUFFER_SEG, 6(%si)

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

	/* LBA read is not supported, so fallback to CHS.  */
	jc	LOCAL(chs_mode)

	movw	$GRUB_BOOT_MACHINE_BUFFER_SEG, %bx
	jmp	LOCAL(copy_buffer)

LOCAL(chs_mode):
	/*
	 *  Determine the hard disk geometry from the BIOS!
	 *  We do this first, so that LS-120 IDE floppies work correctly.
	 */
	movb	$8, %ah
	int	$0x13
	jnc	LOCAL(final_init)

	/*
	 *  The call failed, so maybe use the floppy probe instead.
	 */
	testb	$GRUB_BOOT_MACHINE_BIOS_HD_FLAG, %dl
	jz	LOCAL(floppy_probe)

	/* Nope, we definitely have a hard disk, and we're screwed. */
	ERR(hd_probe_error_string)

LOCAL(final_init):
	/* set the mode to zero */
	movzbl	%dh, %eax
	movb	%ah, -1(%si)

	/* save number of heads */
	incw	%ax
	movl	%eax, 4(%si)

	movzbw	%cl, %dx
	shlw	$2, %dx
	movb	%ch, %al
	movb	%dh, %ah

	/* save number of cylinders */
	incw	%ax
	movw	%ax, 8(%si)

	movzbw	%dl, %ax
	shrb	$2, %al

	/* save number of sectors */
	movl	%eax, (%si)

setup_sectors:
	/* load logical sector start (top half) */
	movl	kernel_sector + 4, %eax

	orl	%eax, %eax
	jnz	LOCAL(geometry_error)

	/* load logical sector start (bottom half) */
	movl	kernel_sector, %eax

	/* zero %edx */
	xorl	%edx, %edx

	/* divide by number of sectors */
	divl	(%si)

	/* save sector start */
	movb	%dl, %cl

	xorw	%dx, %dx	/* zero %edx */
	divl	4(%si)		/* divide by number of heads */

	/* do we need too many cylinders? */
	cmpw	8(%si), %ax
	jge	LOCAL(geometry_error)

	/* normalize sector start (1-based) */
	incb	%cl

	/* low bits of cylinder start */
	movb	%al, %ch

	/* high bits of cylinder start */
	xorb	%al, %al
	shrw	$2, %ax
	orb	%al, %cl

	/* save head start */
	movb	%dl, %al

	/* restore %dl */
	popw	%dx

	/* head start */
	movb	%al, %dh

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
	movw	$0x0201, %ax	/* function 2 */
	int	$0x13

	jc	LOCAL(read_error)

	movw	%es, %bx

LOCAL(copy_buffer):
	/*
	 * We need to save %cx and %si because the startup code in
	 * kernel uses them without initializing them.
	 */
	pusha
	pushw	%ds

	movw	$0x100, %cx
	movw	%bx, %ds
	xorw	%si, %si
	movw	$GRUB_BOOT_MACHINE_KERNEL_ADDR, %di
	movw	%si, %es

	cld

	rep
	movsw

	popw	%ds
	popa

	/* boot kernel */
	jmp	*(kernel_address)

/* END OF MAIN LOOP */

/*
 * BIOS Geometry translation error (past the end of the disk geometry!).
 */
LOCAL(geometry_error):
	ERR(geometry_error_string)

/*
 * Read error on the disk.
 */
LOCAL(read_error):
	movw	$read_error_string, %si
LOCAL(error_message):
	call	LOCAL(message)
LOCAL(general_error):
	MSG(general_error_string)

/* go here when you need to stop the machine hard after an error condition */
        /* tell the BIOS a boot failure, which may result in no effect */
        int	$0x18
LOCAL(stop):
	jmp	LOCAL(stop)

notification_string:	.asciz "GRUB "
geometry_error_string:	.asciz "Geom"
hd_probe_error_string:	.asciz "Hard Disk"
read_error_string:	.asciz "Read"
general_error_string:	.asciz " Error\r\n"

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
LOCAL(message):
	lodsb
	cmpb	$0, %al
	jne	1b	/* if not end of string, jmp to display */
	ret

	/*
	 *  Windows NT breaks compatibility by embedding a magic
	 *  number here.
	 */

	. = _start + GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC
nt_magic:
	.long 0
	.word 0

	/*
	 *  This is where an MBR would go if on a hard disk.  The code
	 *  here isn't even referenced unless we're on a floppy.  Kinda
	 *  sneaky, huh?
	 */

	. = _start + GRUB_BOOT_MACHINE_PART_START
part_start:

probe_values:
	.byte	36, 18, 15, 9, 0

LOCAL(floppy_probe):
/*
 *  Perform floppy probe.
 */

	movw	$probe_values - 1, %si

LOCAL(probe_loop):
	/* reset floppy controller INT 13h AH=0 */
	xorw	%ax, %ax
	int	$0x13

	incw	%si
	movb	(%si), %cl

	/* if number of sectors is 0, display error and die */
	cmpb	$0, %cl
	jne	1f

/*
 * Floppy disk probe failure.
 */
	MSG(fd_probe_error_string)
	jmp	LOCAL(general_error)

/* "Floppy" */
fd_probe_error_string:	.asciz "Floppy"

1:
	/* perform read */
	movw	$GRUB_BOOT_MACHINE_BUFFER_SEG, %bx
	movw	%bx, %es
	xorw	%bx, %bx
	movw	$0x201, %ax
	movb	$0, %ch
	movb	$0, %dh
	int	$0x13

	/* if error, jump to "LOCAL(probe_loop)" */
	jc	LOCAL(probe_loop)

	/* %cl is already the correct value! */
	movb	$1, %dh
	movb	$79, %ch

	jmp	LOCAL(final_init)

	. = _start + GRUB_BOOT_MACHINE_PART_END

/* the last 2 bytes in the sector 0 contain the signature */
	.word	GRUB_BOOT_MACHINE_SIGNATURE
