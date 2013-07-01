/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2004,2005,2006,2007,2008  Free Software Foundation, Inc.
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

#include <grub/machine/memory.h>
#include <grub/machine/int.h>
#include <grub/err.h>
#include <grub/types.h>
#include <grub/misc.h>

struct grub_machine_mmap_entry
{
  grub_uint32_t size;
  grub_uint64_t addr;
  grub_uint64_t len;
#define GRUB_MACHINE_MEMORY_AVAILABLE	1
#define GRUB_MACHINE_MEMORY_RESERVED	2
#define GRUB_MACHINE_MEMORY_ACPI	3
#define GRUB_MACHINE_MEMORY_NVS 	4
#define GRUB_MACHINE_MEMORY_BADRAM 	5
  grub_uint32_t type;
} __attribute__((packed));

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
* @brief 获得传统内存大小。
*
* @note 注释详细内容:
*
* 本函数就是用来获得这部分内存的大小的。 
*
* 我们前面知道，体系结构相关的机器初始化的第3步骤就是调用grub_machine_mmap_iterate()初始化
* 内存区域映射。这个grub_machine_mmap_iterate()函数的原型是在公用的头文件
* 【grub-2.00/include/grub/memory.h】中声明的，但是实际的实现是在各个体系结构中实现的。
* 例如，对于X86的PC平台，该函数是在【grub-2.00/grub-core/kern/i386/pc/mmap.c】实现的。
*
* 为了分析grub_machine_mmap_iterate()，我们必须介绍一下X86的PC架构下内存映射的历史和内存映
* 射的获取办法，以及相应的几个辅助函数。
*
* 首先，在最初阶段，内存很昂贵，并且一般都很小，而且人们几乎没有想到会用到超过1MB的内存，
* 在传统上虽然16位的X86机器可以支持20位地址线（最高可寻址1MB），基本上当时认为640KB的内存
* 也很不错了。因此，最初提供了一个BIOS调用"INT 12H"，能够获得机器的内存，但是只限于最初的
* 640KB。我们现在一般把这部分的内存叫做“传统内存”，或者说“conventional memory”。
*
* 该函数直接使用返回值BIOS call "INT 12H"的EAX返回值，也就是以KB为单位的传统内存大小。
**/

/*
 *
 * grub_get_conv_memsize(i) :  return the conventional memory size in KB.
 *	BIOS call "INT 12H" to get conventional memory size
 *      The return value in AX.
 */
static inline grub_uint16_t
grub_get_conv_memsize (void)
{
  struct grub_bios_int_registers regs;

  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x12, &regs);
  return regs.eax & 0xffff;
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
* @brief 获得扩展内存大小。
*
* @note 注释详细内容:
*
* 本函数使用BIOS调用实现获得扩展内存大小。
*
* 随着保护模式的引入，以及人们写的代码要求的内存使用量越来越多，原有的640KB的限制实在让
* 人无法忍受，因此人们想出了对原有的传统内存进行扩展。这部分内存位于1MB之后，并且由于可
* 以进入32位保护模式来寻址，因此完全可以访问1MB到4GB之间的内存，这样大家使用内存的限制
* 被完全解除。为了获得这一部分 “被扩展”的内存大小，同样也有一个专门的BIOS调用。
**/
/*
 * grub_get_ext_memsize() :  return the extended memory size in KB.
 *	BIOS call "INT 15H, AH=88H" to get extended memory size
 *	The return value in AX.
 *
 */
static inline grub_uint16_t
grub_get_ext_memsize (void)
{
  struct grub_bios_int_registers regs;

  regs.eax = 0x8800;
  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  grub_bios_interrupt (0x15, &regs);
  return regs.eax & 0xffff;
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
* @brief 使用BIOS调用实现EISA内存映射信息。
*
* @note 注释详细内容:
*
* 本函数使用BIOS调用实现EISA内存映射信息。
* 
* 除了上面介绍的区分0~640KB的传统内存，以及1MB以上的扩展内存以外，工业界还曾经引入过EISA
* 内存映射的概念。这一概念把内存进一步分成3个区域，0 - 640K, 1 MB - 16 MB, 16 MB以上。
* 这主要是因为EISA标准的硬件不能访问16MB以上的内存作DMA，所以单独将1MB~16MB区间独立出来
* 探测。为了支持这种内存映射，还单独提供了"INT 15H, AH=E801H"这个BIOS调用。
*
* 该函数将BIOS call "INT 15H, AH=E801H"的返回值的EAX（1MB~16MB之间的以1KB为单位的内存）
* 和EBX（16MB以上以64KB为单位的内存）组合起来，因此调用者需要对这个结果进行拆分。
**/

/* Get a packed EISA memory map. Lower 16 bits are between 1MB and 16MB
   in 1KB parts, and upper 16 bits are above 16MB in 64KB parts. If error, return zero.
   BIOS call "INT 15H, AH=E801H" to get EISA memory map,
     AX = memory between 1M and 16M in 1K parts.
     BX = memory above 16M in 64K parts. 
*/
 
static inline grub_uint32_t
grub_get_eisa_mmap (void)
{
  struct grub_bios_int_registers regs;

  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;
  regs.eax = 0xe801;
  grub_bios_interrupt (0x15, &regs);

  if ((regs.eax & 0xff00) == 0x8600)
    return 0;

  return (regs.eax & 0xffff) | (regs.ebx << 16);
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
* @brief 使用BIOS E820调用获得内存映射信息。
*
* @note 注释详细内容:
*
* 实际上，终极的获取系统内存分布的方法是使用著名的BIOS E820调用，也就是“INT 15H,AH=E820H”
* （即Query System Address Map），而且最新的BIOS几乎都支持这一调用。下面我们简单介绍一下
* 这个BIOS调用（参见http://www.uruk.org/orig-grub/mem64mb.html）。
*
*	AX = E820h的INT 15H - 查询系统地址映射
*
* 这个调用返回所有已安装的RAM的内存映射，以及由BIOS保留的物理内存范围。通过连续调用这个API，
* 每次调用返回一个物理地址的地址映射信息。每次运行时返回的内存属性都具有一个类型，这就决定
* 物理地址范围应如何被操作系统使用。
*
* 如果INT 15h, AX=E820h返回的信息，在某种程度上不同于INT 15H，AX = E801h或 INT 15H AH = 88H
* 返回的信息，那么E820h返回的信息应该取代这些旧接口返回的信息。这使得BIOS由于兼容性的原因
* 可以返回任何信息。
* 
* 这个BIOS调用需要这样实现：
*
* 1）	当第一次调用时，参数寄存器EBX（被称为“continuation value”）应该设置为0；参数寄存器
*     EAX设置为0xE820;参数ES:DI指向地址范围描述符结构，这个结构由BIOS在执行该调用时填入
*     描述地址映射的信息；参数ECX描述传递给BIOS的地址范围描述符的大小（至少为20字节）；
*     参数EDX为一个签名，值为'SMAP'，BIOS用这个签名来验证调用者的确是用这个调用来读取内
*     存映射信息；当执行完后，输出参数要先看CF标志是否设置，没设置的话表示没有出错，才可
*     以继续解析其他输出参数。输出参数大致与输入参数意义一致，但是EBX返回的是一个BIOS特
*     定的值，这个值必须要被下一次调用该BIOS调用时再次传递给输入参数EBX寄存器。如果这个
*     EBX返回的值为0，那么表示已经报告了所有的内存映射，不需要再继续执行了。
* 2）	当第二次，以及后面多次再调用该BIOS调用时，每次传递给EBX寄存器的值应该是前一次调用
*     该BIOS调用返回的EBX寄存器的值。
* 3）	调用者应该以EBX为0开始，循环调用该调用，直到BIOS返回EBX为0，每次调用得到一个内存地
*     址范围描述符。调用者解析这些返回的内存地址范围描述符，来确定地址范围，以及对这些地
*     址范围的使用情况（是否被BIOS保留，如果保留的话这个内存范围就不能被操作系统或者系统
*     软件使用）。
*
* 下面是地址范围描述符的结构描述：
*
* - 字节偏移	名字						描述
*	-	0	    		BaseAddrLow			Low 32 Bits of Base Address
*	-	4	    		BaseAddrHigh		High 32 Bits of Base Address
*	-	8	    		LengthLow				Low 32 Bits of Length in Bytes
*	-	12	    	LengthHigh			High 32 Bits of Length in Bytes
*	-	16	    	Type						Address type of  this range.
*
* - BaseAddrLow	和BaseAddrHigh合起来是内存地址范围的基地址；	
* - LengthLow和LengthHigh合起来是内存地址范围的长度；	
* - Type是内存范围的类型，目前定义了两个值（值1为AddressRangeMemory，表示这段内存可以被系
* 统软件使用；值2为AddressRangeReserved表示这段内存已经被使用或者被系统保留，不能被系统软
* 件使用）。
*
* 该函数按照前面描述的格式填写BIOS E820中断调用参数；其中grub_machine_mmap_entry结构定义在
*【grub-2.00/grub-core/kern/i386/pc/mmap.c】的开始处。
*
* 函数将entry->addr填入ES:DI中，将cont填入EBX中；其他参数也按规范填入；该BIOS调用之后，如
* 果输出参数不符合规范，则将entry->size设置为0，否则将其设置为ECX；最后返回EBX作为
* “continuation value”，用于下一次调用该函数的cont参数。
**/
/*
 *
 * grub_get_mmap_entry(addr, cont) : address and old continuation value (zero to
 *		start), for the Query System Address Map BIOS call.
 *
 *  Sets the first 4-byte int value of "addr" to the size returned by
 *  the call.  If the call fails, sets it to zero.
 *
 *	Returns:  new (non-zero) continuation value, 0 if done.
 */
/* Get a memory map entry. Return next continuation value. Zero means
   the end.  */
static grub_uint32_t
grub_get_mmap_entry (struct grub_machine_mmap_entry *entry,
		     grub_uint32_t cont)
{
  struct grub_bios_int_registers regs;

  regs.flags = GRUB_CPU_INT_FLAGS_DEFAULT;

  /* place address (+4) in ES:DI */
  regs.es = ((grub_addr_t) &entry->addr) >> 4;
  regs.edi = ((grub_addr_t) &entry->addr) & 0xf;
	
  /* set continuation value */
  regs.ebx = cont;

  /* set default maximum buffer size */
  regs.ecx = sizeof (*entry) - sizeof (entry->size);

  /* set EDX to 'SMAP' */
  regs.edx = 0x534d4150;

  regs.eax = 0xe820;
  grub_bios_interrupt (0x15, &regs);

  /* write length of buffer (zero if error) into ADDR */	
  if ((regs.flags & GRUB_CPU_INT_FLAGS_CARRY) || regs.eax != 0x534d4150
      || regs.ecx < 0x14 || regs.ecx > 0x400)
    entry->size = 0;
  else
    entry->size = regs.ecx;

  /* return the continuation value */
  return regs.ebx;
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
* @brief 探测系统内存映射并调用回调函数钩子。
*
* @note 注释详细内容:
*
* 有了所有上面的基础知识之后，我们可以来看grub_machine_mmap_iterate()了，对应X86的PC平台，
* 代码位于【grub-2.00/grub-core/kern/i386/pc/mmap.c】。这个函数的执行流程如下：
*
* 1）	将地址GRUB_MEMORY_MACHINE_SCRATCH_ADDR强制转化为结构体struct grub_machine_mmap_entry
*     类型的指针，并将这片内存清空，从而可以用这片内存区域来作为地址范围描述符的输出区域。
*     这是因为目前阶段还没有探测出系统的内存分布，还没有初始化内存管理器，因此只能用已知的
*     某个可用的内存地址做强制转化来实现结构体等的内存使用。
* 2）	调用grub_get_mmap_entry (entry, 0)来获取第一个内存地址范围描述符；如果这个函数失败，
*     即返回的值cont为0，说明没法使用INT 15H,EAX=E820H这种方法来获得内存地址范围使用信息
*    （这在现在的新系统上几乎都应该成功的），那么就会转到下面的代码。如果成功，则说明可以
*     支持INT 15H,EAX=E820H这种方法，因此，就循环调用该函数；每次调用之后，都会用参数hook()
*     这个函数指针，该回调函数一般就是用来将这个新探测出来的可用内存范围加入到系统内存管理
*     器。每次调用grub_get_mmap_entry()都会把返回值cont作为下一次调用该函数的cont参数，直
*     到该cont返回值为0或者出错（结果表现为entry->size为0）。
* 3）	如果前面发现没法使用INT 15H,EAX=E820H这种方法来获得内存地址范围使用信息，那么就会先
*     执行grub_get_conv_memsize()活动0~640KB直接的传统内存，并使用hook()这个函数指针将这
*     段内存加入系统内存管理器；然后再将调用grub_get_eisa_mmap ()的返回值eisa_mmap解析出来，
*     分别将1MB~16MB之间以及16MB以上的内存使用hook()这个函数指针将这段内存加入系统内存管理
*     器。如果没有eisa_mmap，那么就直接将1MB以上的内存使用hook()这个函数指针将这段内存加入
*     系统内存管理器。
*
* 回过头来再去看【grub-2.00/grub-core/kern/i386/pc/init.c】中的grub_machine_init()中的hook()
* 函数。该函数实际上是忽略所有的起始地址位于1MB以下而内存区域不超过1MB的内存范围（就是所谓
* 的lower memory）；如果起始地址位于1MB以下而内存区域超过1MB的内存范围，那么会把起始地址设
* 置为1MB，而内存范围大小也相应做调整；此外，还会忽略4GB以上的内存（因为目前GRUB2仅仅在X86
* 的保护模式下运行，因此无法利用4GB以上的内存，而且作为bootloader也没必要用这么多内存）；
* 然后再对这些内存范围调用add_mem_region()来添加到系统struct mem_region mem_regions[MAX_REGIONS]
* 这个系统内存区域描述符数组中。
**/
grub_err_t
grub_machine_mmap_iterate (grub_memory_hook_t hook)
{
  grub_uint32_t cont;
  struct grub_machine_mmap_entry *entry
    = (struct grub_machine_mmap_entry *) GRUB_MEMORY_MACHINE_SCRATCH_ADDR;

  grub_memset (entry, 0, sizeof (entry));

  /* Check if grub_get_mmap_entry works.  */
  cont = grub_get_mmap_entry (entry, 0);

  if (entry->size)
    do
      {
	if (hook (entry->addr, entry->len,
		  /* GRUB mmaps have been defined to match with the E820 definition.
		     Therefore, we can just pass type through.  */
		  ((entry->type <= GRUB_MACHINE_MEMORY_BADRAM) && (entry->type >= GRUB_MACHINE_MEMORY_AVAILABLE)) ? entry->type : GRUB_MEMORY_RESERVED))
	  break;

	if (! cont)
	  break;

	grub_memset (entry, 0, sizeof (entry));

	cont = grub_get_mmap_entry (entry, cont);
      }
    while (entry->size);
  else
    {
      grub_uint32_t eisa_mmap = grub_get_eisa_mmap ();

      if (hook (0x0, ((grub_uint32_t) grub_get_conv_memsize ()) << 10,
		GRUB_MEMORY_AVAILABLE))
	return 0;

      if (eisa_mmap)
	{
	  if (hook (0x100000, (eisa_mmap & 0xFFFF) << 10,
		    GRUB_MEMORY_AVAILABLE) == 0)
	    hook (0x1000000, eisa_mmap & ~0xFFFF, GRUB_MEMORY_AVAILABLE);
	}
      else
	hook (0x100000, ((grub_uint32_t) grub_get_ext_memsize ()) << 10,
	      GRUB_MEMORY_AVAILABLE);
    }

  return 0;
}
