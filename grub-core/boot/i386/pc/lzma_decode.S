/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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

#define FIXED_PROPS

#define LZMA_BASE_SIZE 1846
#define LZMA_LIT_SIZE 768

#define LZMA_PROPERTIES_SIZE 5

#define kNumTopBits 24
#define kTopValue (1 << kNumTopBits)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5


#define kNumPosBitsMax 4
#define kNumPosStatesMax (1 << kNumPosBitsMax)

#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumMidBits 3
#define kLenNumMidSymbols (1 << kLenNumMidBits)
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)

#define LenChoice 0
#define LenChoice2 (LenChoice + 1)
#define LenLow (LenChoice2 + 1)
#define LenMid (LenLow + (kNumPosStatesMax << kLenNumLowBits))
#define LenHigh (LenMid + (kNumPosStatesMax << kLenNumMidBits))
#define kNumLenProbs (LenHigh + kLenNumHighSymbols)


#define kNumStates 12
#define kNumLitStates 7

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

#define kNumPosSlotBits 6
#define kNumLenToPosStates 4

#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)

#define kMatchMinLen 2

#define IsMatch 0
#define IsRep (IsMatch + (kNumStates << kNumPosBitsMax))
#define IsRepG0 (IsRep + kNumStates)
#define IsRepG1 (IsRepG0 + kNumStates)
#define IsRepG2 (IsRepG1 + kNumStates)
#define IsRep0Long (IsRepG2 + kNumStates)
#define PosSlot (IsRep0Long + (kNumStates << kNumPosBitsMax))
#define SpecPos (PosSlot + (kNumLenToPosStates << kNumPosSlotBits))
#define Align (SpecPos + kNumFullDistances - kEndPosModelIndex)
#define LenCoder (Align + kAlignTableSize)
#define RepLenCoder (LenCoder + kNumLenProbs)
#define Literal (RepLenCoder + kNumLenProbs)

#define out_size	8(%ebp)

#define now_pos		-4(%ebp)
#define prev_byte	-8(%ebp)
#define range		-12(%ebp)
#define code		-16(%ebp)
#define state		-20(%ebp)
#define rep0		-24(%ebp)
#define rep1		-28(%ebp)
#define rep2		-32(%ebp)
#define rep3		-36(%ebp)

#ifdef FIXED_PROPS

#define FIXED_LC	3
#define FIXED_LP	0
#define FIXED_PB	2

#define POS_STATE_MASK	((1 << (FIXED_PB)) - 1)
#define LIT_POS_MASK	((1 << (FIXED_LP)) - 1)

#define LOCAL_SIZE	36

#else

#define lc		(%ebx)
#define lp		4(%ebx)
#define pb		8(%ebx)
#define probs		12(%ebx)

#define pos_state_mask	-40(%ebp)
#define lit_pos_mask	-44(%ebp)

#define LOCAL_SIZE	44

#endif

RangeDecoderBitDecode:
#ifdef FIXED_PROPS
	leal	(%ebx, %eax, 4), %eax
#else
	shll	$2, %eax
	addl	probs, %eax
#endif

	movl	%eax, %ecx
	movl	(%ecx), %eax

	movl	range, %edx
	shrl	$kNumBitModelTotalBits, %edx
	mull	%edx

	cmpl	code, %eax
	jbe	1f

	movl	%eax, range
	movl	$kBitModelTotal, %edx
	subl	(%ecx), %edx
	shrl	$kNumMoveBits, %edx
	addl	%edx, (%ecx)
	clc
3:
	pushf
	cmpl	$kTopValue, range
	jnc	2f
	shll	$8, code
	lodsb
	movb	%al, code
	shll	$8, range
2:
	popf
	ret
1:
	subl	%eax, range
	subl	%eax, code
	movl	(%ecx), %edx
	shrl	$kNumMoveBits, %edx
	subl	%edx, (%ecx)
	stc
	jmp	3b

RangeDecoderBitTreeDecode:
RangeDecoderReverseBitTreeDecode:
	movzbl	%cl, %ecx
	xorl	%edx, %edx
	pushl	%edx
	incl	%edx
	pushl	%edx

1:
	pushl	%eax
	pushl	%ecx
	pushl	%edx

	addl	%edx, %eax
	call	RangeDecoderBitDecode

	popl	%edx
	popl	%ecx

	jnc	2f
	movl	4(%esp), %eax
	orl	%eax, 8(%esp)
	stc

2:
	adcl	%edx, %edx
	popl	%eax

	shll	$1, (%esp)
	loop	1b

	popl	%ecx
	subl	%ecx, %edx		/* RangeDecoderBitTreeDecode */
	popl	%ecx			/* RangeDecoderReverseBitTreeDecode */
	ret

LzmaLenDecode:
	pushl	%eax
	addl	$LenChoice, %eax
	call	RangeDecoderBitDecode
	popl	%eax
	jc	1f
	pushl	$0
	movb	$kLenNumLowBits, %cl
	addl	$LenLow, %eax
2:
	movl	12(%esp), %edx
	shll	%cl, %edx
	addl	%edx, %eax
3:

	call	RangeDecoderBitTreeDecode
	popl	%eax
	addl	%eax, %edx
	ret

1:
	pushl	%eax
	addl	$LenChoice2, %eax
	call	RangeDecoderBitDecode
	popl	%eax
	jc	1f
	pushl	$kLenNumLowSymbols
	movb	$kLenNumMidBits, %cl
	addl	$LenMid, %eax
	jmp	2b

1:
	pushl	$(kLenNumLowSymbols + kLenNumMidSymbols)
	addl	$LenHigh, %eax
	movb	$kLenNumHighBits, %cl
	jmp	3b

WriteByte:
	movb	%al, prev_byte
	stosb
	incl	now_pos
	ret

/*
 * int LzmaDecode(CLzmaDecoderState *vs,
 *                const unsigned char *inStream,
 *                unsigned char *outStream,
 *                SizeT outSize);
 */

_LzmaDecodeA:

	pushl	%ebp
	movl	%esp, %ebp
	subl	$LOCAL_SIZE, %esp

#ifndef ASM_FILE
	pushl	%esi
	pushl	%edi
	pushl	%ebx

	movl	%eax, %ebx
	movl	%edx, %esi
	pushl	%ecx
#else
	pushl	%edi
#endif

	cld

#ifdef FIXED_PROPS
	movl	%ebx, %edi
	movl	$(Literal + (LZMA_LIT_SIZE << (FIXED_LC + FIXED_LP))), %ecx
#else
	movl	$LZMA_LIT_SIZE, %eax
	movb	lc, %cl
	addb	lp, %cl
	shll	%cl, %eax
	addl	$Literal, %eax
	movl	%eax, %ecx
	movl	probs, %edi
#endif

	movl	$(kBitModelTotal >> 1), %eax

	rep
	stosl

	popl	%edi

	xorl	%eax, %eax
	movl	%eax, now_pos
	movl	%eax, prev_byte
	movl	%eax, state

	incl	%eax
	movl	%eax, rep0
	movl	%eax, rep1
	movl	%eax, rep2
	movl	%eax, rep3

#ifndef FIXED_PROPS
	movl	%eax, %edx
	movb	pb, %cl
	shll	%cl, %edx
	decl	%edx
	movl	%edx, pos_state_mask

	movl	%eax, %edx
	movb	lp, %cl
	shll	%cl, %edx
	decl	%edx
	movl	%edx, lit_pos_mask;
#endif

	/* RangeDecoderInit */
	negl	%eax
	movl	%eax, range

	incl	%eax
	movb	$5, %cl

1:
	shll	$8, %eax
	lodsb
	loop	1b

	movl	%eax, code

lzma_decode_loop:
	movl	now_pos, %eax
	cmpl	out_size, %eax

	jb	1f

#ifndef ASM_FILE
	xorl	%eax, %eax

	popl	%ebx
	popl	%edi
	popl	%esi
#endif

	movl	%ebp, %esp
	popl	%ebp
	ret

1:
#ifdef FIXED_PROPS
	andl	$POS_STATE_MASK, %eax
#else
	andl	pos_state_mask, %eax
#endif
	pushl	%eax				/* posState */
	movl	state, %edx
	shll	$kNumPosBitsMax, %edx
	addl	%edx, %eax
	pushl	%eax				/* (state << kNumPosBitsMax) + posState */

	call	RangeDecoderBitDecode
	jc	1f

	movl	now_pos, %eax

#ifdef FIXED_PROPS
	andl	$LIT_POS_MASK, %eax
	shll	$FIXED_LC, %eax
	movl	prev_byte, %edx
	shrl	$(8 - FIXED_LC), %edx
#else
	andl	lit_pos_mask, %eax
	movb	lc, %cl
	shll	%cl, %eax
	negb	%cl
	addb	$8, %cl
	movl	prev_byte, %edx
	shrl	%cl, %edx
#endif

	addl	%edx, %eax
	movl	$LZMA_LIT_SIZE, %edx
	mull	%edx
	addl	$Literal, %eax
	pushl	%eax

	incl	%edx			/* edx = 1 */

	movl	rep0, %eax
	negl	%eax
	pushl	(%edi, %eax)		/* matchByte */

	cmpb	$kNumLitStates, state
	jb	5f

	/* LzmaLiteralDecodeMatch */

3:
	cmpl	$0x100, %edx
	jae	4f

	xorl	%eax, %eax
	shlb	$1, (%esp)
	adcl	%eax, %eax

	pushl	%eax
	pushl	%edx

	shll	$8, %eax
	leal	0x100(%edx, %eax), %eax
	addl	12(%esp), %eax
	call	RangeDecoderBitDecode

	setc	%al
	popl	%edx
	adcl	%edx, %edx

	popl	%ecx
	cmpb	%cl, %al
	jz	3b

5:

	/* LzmaLiteralDecode */

	cmpl	$0x100, %edx
	jae	4f

	pushl	%edx
	movl	%edx, %eax
	addl	8(%esp), %eax
	call	RangeDecoderBitDecode
	popl	%edx
	adcl	%edx, %edx
	jmp	5b

4:
	addl	$16, %esp

	movb	%dl, %al
	call	WriteByte

	movb	state, %al
	cmpb	$4, %al
	jae	2f
	xorb	%al, %al
	jmp	3f
2:
	subb	$3, %al
	cmpb	$7, %al
	jb	3f
	subb	$3, %al
3:
	movb	%al, state
	jmp	lzma_decode_loop

1:
	movl	state, %eax
	addl	$IsRep, %eax
	call	RangeDecoderBitDecode
	jnc	1f

	movl	state, %eax
	addl	$IsRepG0, %eax
	call	RangeDecoderBitDecode
	jc	10f

	movl	(%esp), %eax
	addl	$IsRep0Long, %eax
	call	RangeDecoderBitDecode
	jc	20f

	cmpb	$7, state
	movb	$9, state
	jb	100f
	addb	$2, state
100:

	movl	$1, %ecx

3:
	movl	rep0, %edx
	negl	%edx

4:
	movb	(%edi, %edx), %al
	call	WriteByte
	loop	4b

	popl	%eax
	popl	%eax
	jmp	lzma_decode_loop

10:
	movl	state, %eax
	addl	$IsRepG1, %eax
	call	RangeDecoderBitDecode
	movl	rep1, %edx
	jnc	100f

	movl	state, %eax
	addl	$IsRepG2, %eax
	call	RangeDecoderBitDecode
	movl	rep2, %edx
	jnc	1000f
	movl	rep2, %edx
	xchgl	rep3, %edx
1000:
	pushl	rep1
	popl	rep2
100:
	xchg	rep0, %edx
	movl	%edx, rep1
20:

	movl	$RepLenCoder, %eax
	call	LzmaLenDecode

	cmpb	$7, state
	movb	$8, state
	jb	100f
	addb	$3, state
100:
	jmp	2f

1:
	movl	rep0, %eax
	xchgl	rep1, %eax
	xchgl	rep2, %eax
	movl	%eax, rep3

	cmpb	$7, state
	movb	$7, state
	jb	10f
	addb	$3, state
10:

	movl	$LenCoder, %eax
	call	LzmaLenDecode
	pushl	%edx

	movl	$(kNumLenToPosStates - 1), %eax
	cmpl	%eax, %edx
	jbe	100f
	movl	%eax, %edx
100:
	movb	$kNumPosSlotBits, %cl
	shll	%cl, %edx
	leal	PosSlot(%edx), %eax
	call	RangeDecoderBitTreeDecode

	movl	%edx, rep0
	cmpl	$kStartPosModelIndex, %edx
	jb	100f

	movl	%edx, %ecx
	shrl	$1, %ecx
	decl	%ecx

	movzbl	%dl, %eax
	andb	$1, %al
	orb	$2, %al
	shll	%cl, %eax
	movl	%eax, rep0

	cmpl	$kEndPosModelIndex, %edx
	jae	200f
	movl	rep0, %eax
	addl	$(SpecPos - 1), %eax
	subl	%edx, %eax
	jmp	300f
200:

	subb	$kNumAlignBits, %cl

	/* RangeDecoderDecodeDirectBits */
	xorl	%edx, %edx

1000:
	shrl	$1, range
	shll	$1, %edx

	movl	range, %eax
	cmpl	%eax, code
	jb	2000f
	subl	%eax, code
	orb	$1, %dl
2000:

	cmpl	$kTopValue, %eax
	jae	3000f
	shll	$8, range
	shll	$8, code
	lodsb
	movb	%al, code

3000:
	loop	1000b

	movb	$kNumAlignBits, %cl
	shll	%cl, %edx
	addl	%edx, rep0

	movl	$Align, %eax

300:
	call	RangeDecoderReverseBitTreeDecode
	addl	%ecx, rep0

100:
	incl	rep0
	popl	%edx

2:

	addl	$kMatchMinLen, %edx
	movl	%edx, %ecx

	jmp	3b
