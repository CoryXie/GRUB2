/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010  Free Software Foundation, Inc.
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

#include <grub/kernel.h>
#include <grub/mm.h>
#include <grub/machine/boot.h>
#include <grub/i386/floppy.h>
#include <grub/machine/memory.h>
#include <grub/machine/console.h>
#include <grub/machine/kernel.h>
#include <grub/machine/int.h>
#include <grub/types.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/loader.h>
#include <grub/env.h>
#include <grub/cache.h>
#include <grub/time.h>
#include <grub/cpu/tsc.h>
#include <grub/machine/time.h>

struct mem_region
{
  grub_addr_t addr;
  grub_size_t size;
};

#define MAX_REGIONS	32

static struct mem_region mem_regions[MAX_REGIONS];
static int num_regions;

void (*grub_pc_net_config) (char **device, char **path);

/*
 *	return the real time in ticks, of which there are about
 *	18-20 per second
 */
grub_uint32_t
grub_get_rtc (void)
{
  struct grub_bios_int_registers regs;

  regs.eax = 0;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x1a, &regs);

  return (regs.ecx << 16) | (regs.edx & 0xffff);
}

void
grub_machine_get_bootlocation (char **device, char **path)
{
  char *ptr;
  grub_uint8_t boot_drive, dos_part, bsd_part;

  boot_drive = (grub_boot_device >> 24);
  dos_part = (grub_boot_device >> 16);
  bsd_part = (grub_boot_device >> 8);

  /* No hardcoded root partition - make it from the boot drive and the
     partition number encoded at the install time.  */
  if (boot_drive == GRUB_BOOT_MACHINE_PXE_DL)
    {
      if (grub_pc_net_config)
	grub_pc_net_config (device, path);
      return;
    }

  /* XXX: This should be enough.  */
#define DEV_SIZE 100
  *device = grub_malloc (DEV_SIZE);
  ptr = *device;
  grub_snprintf (*device, DEV_SIZE,
		 "%cd%u", (boot_drive & 0x80) ? 'h' : 'f',
		 boot_drive & 0x7f);
  ptr += grub_strlen (ptr);

  if (dos_part != 0xff)
    grub_snprintf (ptr, DEV_SIZE - (ptr - *device),
		   ",%u", dos_part + 1);
  ptr += grub_strlen (ptr);

  if (bsd_part != 0xff)
    grub_snprintf (ptr, DEV_SIZE - (ptr - *device), ",%u",
		   bsd_part + 1);
  ptr += grub_strlen (ptr);
  *ptr = 0;
}

/* Add a memory region.  */
static void
add_mem_region (grub_addr_t addr, grub_size_t size)
{
  if (num_regions == MAX_REGIONS)
    /* Ignore.  */
    return;

  mem_regions[num_regions].addr = addr;
  mem_regions[num_regions].size = size;
  num_regions++;
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
* @brief 紧凑内存分布描述符数组。
*
* @note 注释详细内容:
*
* 这个函数实现紧凑内存分布描述符数组。该函数执行下列步骤：
* 
* 1）	首先将mem_regions[]数组安装addr大小排序；
* 2）	然后按照顺序，依次对比前一个和后一个mem_regions[]数组元素，如果发现有重叠的现象，
*     则将这两个区域再次合并。
* 
* 这样得出的mem_regions[]数组是一个最紧凑的内存分布描述符数组。对这个数组里面的每一项，
* 都会调用 grub_mm_init_region()来将这片内存区域初始化成由一个grub_mm_region_t管理的空
* 闲内存区域。有了这个初始化，GRUB2后面的代码就可以使用grub_malloc()和grub_free()等内
* 存分配函数了。
**/
/* Compact memory regions.  */
static void
compact_mem_regions (void)
{
  int i, j;

  /* Sort them.  */
  for (i = 0; i < num_regions - 1; i++)
    for (j = i + 1; j < num_regions; j++)
      if (mem_regions[i].addr > mem_regions[j].addr)
	{
	  struct mem_region tmp = mem_regions[i];
	  mem_regions[i] = mem_regions[j];
	  mem_regions[j] = tmp;
	}

  /* Merge overlaps.  */
  for (i = 0; i < num_regions - 1; i++)
    if (mem_regions[i].addr + mem_regions[i].size >= mem_regions[i + 1].addr)
      {
	j = i + 1;

	if (mem_regions[i].addr + mem_regions[i].size
	    < mem_regions[j].addr + mem_regions[j].size)
	  mem_regions[i].size = (mem_regions[j].addr + mem_regions[j].size
				 - mem_regions[i].addr);

	grub_memmove (mem_regions + j, mem_regions + j + 1,
		      (num_regions - j - 1) * sizeof (struct mem_region));
	i--;
        num_regions--;
      }
}

grub_addr_t grub_modbase;
extern grub_uint8_t _start[], _edata[];

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
* @brief 体系结构特定的初始化。
*
* @note 注释详细内容:
*
* grub_main()首先调用grub_machine_init()，这是为了实现对不同体系结构，以及对于同一硬件体
* 系结构而使用不同Firmware固件的平台进行抽象，因为每种体系结构都有特定的内存映射等特殊
* 设置。例如【grub-2.00/grub-core/kern/i386/pc/init.c】就对X86架构的PC系列有特殊的初始
* 化（这是基于传统BIOS的PC），而对于基于EFI启动的X86架构的PC，也有对应的另一套初始化代码
*【grub-2.00/grub-core/kern/i386/efi/init.c】，此外对于使用coreboot作为Firmware的X86平台，
* 使用的是【grub-2.00/grub-core/kern/i386/coreboot/init.c】中的机器初始化代码，而这几种
* 都是针对X86架构的不同固件模式进行的支持。对于MIPS的机器，类似也有针对QEMU仿真机器的
* 【grub-2.00/grub-core/kern/mips/qemu_mips/init.c】以及针对龙芯电脑的机器初始化代码
* 【grub-2.00/grub-core/kern/mips/loongson/init.c】。
*
* 我们目前先关注X86架构PC的初始化代码，即【grub-2.00/grub-core/kern/i386/pc/init.c】文件
* 中的grub_machine_init()。
* 
* 这段代码完成了以下功能：
* 
* 1）	将模块结构数组的基地址grub_modbase设置到GRUB核心被解压缩的内存区域的代码段的后面，
*     即GRUB_MEMORY_MACHINE_DECOMPRESSION_ADDR + (_edata - _start)。
* 2）	调用grub_console_init()初始化控制台。
* 3）	调用grub_machine_mmap_iterate()初始化内存区域映射。并调用compact_mem_regions()来将
*     内存区域映射紧凑化（对内存区域进行排序，并合并有重叠的区域）。
* 4）	对每个内存区域，分别调用grub_mm_init_region()来初始化内存管理的区域，从而可以使用
*     这些内存区域做内存分配。
* 5）	调用grub_tsc_init()初始化时间戳，从而有了时间度量的基础。
**/
void
grub_machine_init (void)
{
  int i;
#if 0
  int grub_lower_mem;
#endif

  grub_modbase = GRUB_MEMORY_MACHINE_DECOMPRESSION_ADDR + (_edata - _start);

  /* Initialize the console as early as possible.  */
  grub_console_init ();

  /* This sanity check is useless since top of GRUB_MEMORY_MACHINE_RESERVED_END
     is used for stack and if it's unavailable we wouldn't have gotten so far.
   */
#if 0
  grub_lower_mem = grub_get_conv_memsize () << 10;

  /* Sanity check.  */
  if (grub_lower_mem < GRUB_MEMORY_MACHINE_RESERVED_END)
    grub_fatal ("too small memory");
#endif

/* FIXME: This prevents loader/i386/linux.c from using low memory.  When our
   heap implements support for requesting a chunk in low memory, this should
   no longer be a problem.  */
#if 0
  /* Add the lower memory into free memory.  */
  if (grub_lower_mem >= GRUB_MEMORY_MACHINE_RESERVED_END)
    add_mem_region (GRUB_MEMORY_MACHINE_RESERVED_END,
		    grub_lower_mem - GRUB_MEMORY_MACHINE_RESERVED_END);
#endif

  auto int NESTED_FUNC_ATTR hook (grub_uint64_t, grub_uint64_t,
				  grub_memory_type_t);
  int NESTED_FUNC_ATTR hook (grub_uint64_t addr, grub_uint64_t size,
			     grub_memory_type_t type)
    {
      /* Avoid the lower memory.  */
      if (addr < 0x100000)
	{
	  if (size <= 0x100000 - addr)
	    return 0;

	  size -= 0x100000 - addr;
	  addr = 0x100000;
	}

      /* Ignore >4GB.  */
      if (addr <= 0xFFFFFFFF && type == GRUB_MEMORY_AVAILABLE)
	{
	  grub_size_t len;

	  len = (grub_size_t) ((addr + size > 0xFFFFFFFF)
		 ? 0xFFFFFFFF - addr
		 : size);
	  add_mem_region (addr, len);
	}

      return 0;
    }

  grub_machine_mmap_iterate (hook);

  compact_mem_regions ();

  for (i = 0; i < num_regions; i++)
      grub_mm_init_region ((void *) mem_regions[i].addr, mem_regions[i].size);

  grub_tsc_init ();
}

void
grub_machine_fini (void)
{
  grub_console_fini ();
  grub_stop_floppy ();
}
