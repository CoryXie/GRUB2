/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/mm.h>
#include <grub/misc.h>

#include <grub/types.h>
#include <grub/err.h>
#include <grub/term.h>

#include <grub/i386/relocator.h>
#include <grub/relocator_private.h>
#include <grub/i386/relocator_private.h>
#include <grub/i386/pc/int.h>

extern grub_uint8_t grub_relocator_forward_start;
extern grub_uint8_t grub_relocator_forward_end;
extern grub_uint8_t grub_relocator_backward_start;
extern grub_uint8_t grub_relocator_backward_end;

extern void *grub_relocator_backward_dest;
extern void *grub_relocator_backward_src;
extern grub_size_t grub_relocator_backward_chunk_size;

extern void *grub_relocator_forward_dest;
extern void *grub_relocator_forward_src;
extern grub_size_t grub_relocator_forward_chunk_size;

extern grub_uint8_t grub_relocator16_start;
extern grub_uint8_t grub_relocator16_end;
extern grub_uint16_t grub_relocator16_cs;
extern grub_uint16_t grub_relocator16_ip;
extern grub_uint16_t grub_relocator16_ds;
extern grub_uint16_t grub_relocator16_es;
extern grub_uint16_t grub_relocator16_fs;
extern grub_uint16_t grub_relocator16_gs;
extern grub_uint16_t grub_relocator16_ss;
extern grub_uint16_t grub_relocator16_sp;
extern grub_uint32_t grub_relocator16_edx;
extern grub_uint32_t grub_relocator16_ebx;
extern grub_uint32_t grub_relocator16_esi;

extern grub_uint16_t grub_relocator16_keep_a20_enabled;

extern grub_uint8_t grub_relocator32_start;
extern grub_uint8_t grub_relocator32_end;
extern grub_uint32_t grub_relocator32_eax;
extern grub_uint32_t grub_relocator32_ebx;
extern grub_uint32_t grub_relocator32_ecx;
extern grub_uint32_t grub_relocator32_edx;
extern grub_uint32_t grub_relocator32_eip;
extern grub_uint32_t grub_relocator32_esp;
extern grub_uint32_t grub_relocator32_ebp;
extern grub_uint32_t grub_relocator32_esi;
extern grub_uint32_t grub_relocator32_edi;

extern grub_uint8_t grub_relocator64_start;
extern grub_uint8_t grub_relocator64_end;
extern grub_uint64_t grub_relocator64_rax;
extern grub_uint64_t grub_relocator64_rbx;
extern grub_uint64_t grub_relocator64_rcx;
extern grub_uint64_t grub_relocator64_rdx;
extern grub_uint64_t grub_relocator64_rip;
extern grub_uint64_t grub_relocator64_rip_addr;
extern grub_uint64_t grub_relocator64_rsp;
extern grub_uint64_t grub_relocator64_rsi;
extern grub_addr_t grub_relocator64_cr3;
extern struct grub_i386_idt grub_relocator16_idt;

#define RELOCATOR_SIZEOF(x)	(&grub_relocator##x##_end - &grub_relocator##x##_start)

grub_size_t grub_relocator_align = 1;
grub_size_t grub_relocator_forward_size;
grub_size_t grub_relocator_backward_size;
#ifdef __x86_64__
grub_size_t grub_relocator_jumper_size = 12;
#else
grub_size_t grub_relocator_jumper_size = 7;
#endif

void
grub_cpu_relocator_init (void)
{
  grub_relocator_forward_size = RELOCATOR_SIZEOF(_forward);
  grub_relocator_backward_size = RELOCATOR_SIZEOF(_backward);
}

void
grub_cpu_relocator_jumper (void *rels, grub_addr_t addr)
{
  grub_uint8_t *ptr;
  ptr = rels;
#ifdef __x86_64__
  /* movq imm64, %rax (for relocator) */
  *(grub_uint8_t *) ptr = 0x48;
  ptr++;
  *(grub_uint8_t *) ptr = 0xb8;
  ptr++;
  *(grub_uint64_t *) ptr = addr;
  ptr += sizeof (grub_uint64_t);
#else
  /* movl imm32, %eax (for relocator) */
  *(grub_uint8_t *) ptr = 0xb8;
  ptr++;
  *(grub_uint32_t *) ptr = addr;
  ptr += sizeof (grub_uint32_t);
#endif
  /* jmp $eax/$rax */
  *(grub_uint8_t *) ptr = 0xff;
  ptr++;
  *(grub_uint8_t *) ptr = 0xe0;
  ptr++;
}

void
grub_cpu_relocator_backward (void *ptr, void *src, void *dest,
			     grub_size_t size)
{
  grub_relocator_backward_dest = dest;
  grub_relocator_backward_src = src;
  grub_relocator_backward_chunk_size = size;

  grub_memmove (ptr,
		&grub_relocator_backward_start,
		RELOCATOR_SIZEOF (_backward));
}

void
grub_cpu_relocator_forward (void *ptr, void *src, void *dest,
			     grub_size_t size)
{
  grub_relocator_forward_dest = dest;
  grub_relocator_forward_src = src;
  grub_relocator_forward_chunk_size = size;

  grub_memmove (ptr,
		&grub_relocator_forward_start,
		RELOCATOR_SIZEOF (_forward));
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月28日
*
* @brief 启动内核映像
*
* @note 注释详细内容: GRUB2启动 Linux过程分析
*
* 我们下面看看这段代码如何实现Linux内核的启动的。
*
* 1）	调用grub_relocator_alloc_chunk_align()分配一段内存，返回其地址在变量ch中。
* 2）	根据输入填写几个变量，例如grub_relocator32_eip，grub_relocator32_esp，
*     grub_relocator32_ebp等等，看似平常，却颇有深意。我们前面看到了
*     state.eip = params->code32_start，以及state.esp = real_mode_target；这里就将
*     grub_relocator32_eip设置为params->code32_start，也就是在我们调用grub_cmd_linux()
*     读入Linux内核映像时，得到的内核的32位入口地址。而像grub_relocator32_esp这些变量
*     是哪里来的呢？grub-2.00/grub-core/lib/i386/relocator32.S中。而在
*     grub_relocator32_esp()中的grub_relocator32_esp变量实际上是通过extern声明的，即
*     extern grub_uint32_t grub_relocator32_esp;因此，当我们设置grub_relocator32_esp
*     = state.esp时，实际就是将上面relocator32.S中的VARIABLE(grub_relocator32_esp)处的
*     “.long	0”地址填上了state.esp的值。我们纵观relocator32.S文件，发现前面所有赋值的
*     变量都是类似的。因此，当上面的几行赋值完成后，relocator32.S文件中对应的那些“.long	0”
*     都已经被赋值了恰当的值。
* 3）	调用一条grub_memmove()函数，将grub_relocator32_start变量地址处的一片内存move到前
*     面分配的内存中；而grub_relocator32_start变量实际上是relocator32.S中代码开始出的
*     一个标号。
* 
* VARIABLE(grub_relocator32_start)
*			PREAMBLE
*
*     同样还有该文件结尾处的标号VARIABLE(grub_relocator32_end)。因此，我们知
*     道这个grub_memmove()调用实际上是把relocator32.S中的代码（修正后）搬移到新分配的
*     内存处。
* 4）	调用grub_relocator_prepare_relocs()将代码重定位到返回地址relst处。
* 5）	最后将relst转换成函数指针直接调用：((void (*) (void)) relst) ();这样也就是实现了
*     跳转到relocator32.S中的grub_relocator32_start处。
* 6）	我们可以进一步分析relocator32.S的代码，大致如下：
*     -	调用宏PREAMBLE
*     -	调用宏RELOAD_GDT
*     -	调用宏DISABLE_PAGING
*     -	关闭PAE (物理地址扩展)
*     -	将esp，ebp，esi，edi，eax，ebx，ecx，edx等寄存器的值从前面赋值的变量处加载到寄存器中
*     -	最后一段,是如下代码：
*
*  .byte	   0xea
*  VARIABLE(grub_relocator32_eip)
*	 .long	0
*	 .word	CODE_SEGMENT
* 
* 我们通过对x86汇编指令集的对比可以发现，这里其实是一句跳转语句的直接编码，即
* JMPF CODE_SEGMENT:grub_relocator32_eip。因此，我们在这里其实是一个远跳转，所跳转的目的地址，
* 其实就是我们在加载Linux内核映像时的获得的32位代码入口code32_start。我们可以查看Linux的源
* 代码来和这里的分析对应，见【linux/arch/x86/boot/header.S】。
*
* 因此，我们实际上是跳转到了内核在这个地址处填的值，默认是0x100000。这也就是在我们加载内核的
* 时候，需要获得一个preffered_address，而这个地址默认是被设置为 GRUB_LINUX_BZIMAGE_ADDR，
* 而GRUB_LINUX_BZIMAGE_ADDR的值恰好就是0x100000的原因。也就是说，我们终于勇敢一跳，跳到了内
* 核被加载的内存地址，开始了内核的执行。
*
* 到此为止，我们可以和GRUB说再见了。
**/
grub_err_t
grub_relocator32_boot (struct grub_relocator *rel,
		       struct grub_relocator32_state state,
		       int avoid_efi_bootservices)
{
  grub_err_t err;
  void *relst;
  grub_relocator_chunk_t ch;

  err = grub_relocator_alloc_chunk_align (rel, &ch, 0,
					  (0xffffffff - RELOCATOR_SIZEOF (32))
					  + 1, RELOCATOR_SIZEOF (32), 16,
					  GRUB_RELOCATOR_PREFERENCE_NONE,
					  avoid_efi_bootservices);
  if (err)
    return err;

  grub_relocator32_eax = state.eax;
  grub_relocator32_ebx = state.ebx;
  grub_relocator32_ecx = state.ecx;
  grub_relocator32_edx = state.edx;
  grub_relocator32_eip = state.eip;
  grub_relocator32_esp = state.esp;
  grub_relocator32_ebp = state.ebp;
  grub_relocator32_esi = state.esi;
  grub_relocator32_edi = state.edi;

  grub_memmove (get_virtual_current_address (ch), &grub_relocator32_start,
		RELOCATOR_SIZEOF (32));

  err = grub_relocator_prepare_relocs (rel, get_physical_target_address (ch),
				       &relst, NULL);
  if (err)
    return err;

  asm volatile ("cli");
  ((void (*) (void)) relst) ();

  /* Not reached.  */
  return GRUB_ERR_NONE;
}

grub_err_t
grub_relocator16_boot (struct grub_relocator *rel,
		       struct grub_relocator16_state state)
{
  grub_err_t err;
  void *relst;
  grub_relocator_chunk_t ch;

  /* Put it higher than the byte it checks for A20 check.  */
  err = grub_relocator_alloc_chunk_align (rel, &ch, 0x8010,
					  0xa0000 - RELOCATOR_SIZEOF (16)
					  - GRUB_RELOCATOR16_STACK_SIZE,
					  RELOCATOR_SIZEOF (16)
					  + GRUB_RELOCATOR16_STACK_SIZE, 16,
					  GRUB_RELOCATOR_PREFERENCE_NONE,
					  0);
  if (err)
    return err;

  grub_relocator16_cs = state.cs;  
  grub_relocator16_ip = state.ip;

  grub_relocator16_ds = state.ds;
  grub_relocator16_es = state.es;
  grub_relocator16_fs = state.fs;
  grub_relocator16_gs = state.gs;

  grub_relocator16_ss = state.ss;
  grub_relocator16_sp = state.sp;

  grub_relocator16_ebx = state.ebx;
  grub_relocator16_edx = state.edx;
  grub_relocator16_esi = state.esi;
#ifdef GRUB_MACHINE_PCBIOS
  grub_relocator16_idt = *grub_realidt;
#else
  grub_relocator16_idt.base = 0;
  grub_relocator16_idt.limit = 0;
#endif

  grub_relocator16_keep_a20_enabled = state.a20;

  grub_memmove (get_virtual_current_address (ch), &grub_relocator16_start,
		RELOCATOR_SIZEOF (16));

  err = grub_relocator_prepare_relocs (rel, get_physical_target_address (ch),
				       &relst, NULL);
  if (err)
    return err;

  asm volatile ("cli");
  ((void (*) (void)) relst) ();

  /* Not reached.  */
  return GRUB_ERR_NONE;
}

grub_err_t
grub_relocator64_boot (struct grub_relocator *rel,
		       struct grub_relocator64_state state,
		       grub_addr_t min_addr, grub_addr_t max_addr)
{
  grub_err_t err;
  void *relst;
  grub_relocator_chunk_t ch;

  err = grub_relocator_alloc_chunk_align (rel, &ch, min_addr,
					  max_addr - RELOCATOR_SIZEOF (64),
					  RELOCATOR_SIZEOF (64), 16,
					  GRUB_RELOCATOR_PREFERENCE_NONE,
					  0);
  if (err)
    return err;

  grub_relocator64_rax = state.rax;
  grub_relocator64_rbx = state.rbx;
  grub_relocator64_rcx = state.rcx;
  grub_relocator64_rdx = state.rdx;
  grub_relocator64_rip = state.rip;
  grub_relocator64_rsp = state.rsp;
  grub_relocator64_rsi = state.rsi;
  grub_relocator64_cr3 = state.cr3;

  grub_memmove (get_virtual_current_address (ch), &grub_relocator64_start,
		RELOCATOR_SIZEOF (64));

  err = grub_relocator_prepare_relocs (rel, get_physical_target_address (ch),
				       &relst, NULL);
  if (err)
    return err;

  asm volatile ("cli");
  ((void (*) (void)) relst) ();

  /* Not reached.  */
  return GRUB_ERR_NONE;
}
