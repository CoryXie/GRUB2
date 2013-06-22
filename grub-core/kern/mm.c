/* mm.c - functions for memory manager */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2007,2008,2009  Free Software Foundation, Inc.
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
  The design of this memory manager.

  This is a simple implementation of malloc with a few extensions. These are
  the extensions:

  - memalign is implemented efficiently.

  - multiple regions may be used as free space. They may not be
  contiguous.

  Regions are managed by a singly linked list, and the meta information is
  stored in the beginning of each region. Space after the meta information
  is used to allocate memory.

  The memory space is used as cells instead of bytes for simplicity. This
  is important for some CPUs which may not access multiple bytes at a time
  when the first byte is not aligned at a certain boundary (typically,
  4-byte or 8-byte). The size of each cell is equal to the size of struct
  grub_mm_header, so the header of each allocated/free block fits into one
  cell precisely. One cell is 16 bytes on 32-bit platforms and 32 bytes
  on 64-bit platforms.

  There are two types of blocks: allocated blocks and free blocks.

  In allocated blocks, the header of each block has only its size. Note that
  this size is based on cells but not on bytes. The header is located right
  before the returned pointer, that is, the header resides at the previous
  cell.

  Free blocks constitutes a ring, using a singly linked list. The first free
  block is pointed to by the meta information of a region. The allocator
  attempts to pick up the second block instead of the first one. This is
  a typical optimization against defragmentation, and makes the
  implementation a bit easier.

  For safety, both allocated blocks and free ones are marked by magic
  numbers. Whenever anything unexpected is detected, GRUB aborts the
  operation.
 */

#include <config.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/err.h>
#include <grub/types.h>
#include <grub/disk.h>
#include <grub/dl.h>
#include <grub/i18n.h>
#include <grub/mm_private.h>

#ifdef MM_DEBUG
# undef grub_malloc
# undef grub_zalloc
# undef grub_realloc
# undef grub_free
# undef grub_memalign
#endif

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月21日
*
* @brief GRUB2内存管理简介。
*
* @note 注释详细内容:
*
* GRUB2核心内存管理器从BIOS取得计算机的内存配置，并串接在区域内存(grub_mm_region)链表，
* 链表头为grub_mm_base。每块内存区域可以分配成很多小块内存，并由内存头(grub_mm_header)
* 将其链结成小块内存链表，链表头位于每块区域内存的最前面。当模块载入时，模块管理器从
* 内存管理器取得内存，将模块的文件内容链结到核心文件结构。核心内存管理器文件是
* grub-2.00/grub-core/kern/mm.c。内存分布图和计算机架构有关，以I386-PC架构来说，就是
* grub-2.00/grub-core/kern/i386/pc/mmap.c。至于grub-2.00/grub-core/mmap的mmap模块则是需
* 要透过模块加载过程后才能使用的mmap命令。
**/

grub_mm_region_t grub_mm_base;

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月21日
*
* @brief 获得已分配内存对应的头部(grub_mm_header_t)和区域(grub_mm_region_t)指针。
*
* @note 注释详细内容:
*
* 本函数实现获得已分配内存对应的头部(grub_mm_header_t)和区域(grub_mm_region_t)指针的功能。
* 函数首先判断参数ptr是否落在某个以grub_mm_base开头的grub_mm_region_t区域内(并保存在r中)；
* 接着将ptr减去grub_mm_header_t结构大小，就得到对应的grub_mm_header_t指针(并保存在p中)。
**/
/* Get a header from the pointer PTR, and set *P and *R to a pointer
   to the header and a pointer to its region, respectively. PTR must
   be allocated.  */
static void
get_header_from_pointer (void *ptr, grub_mm_header_t *p, grub_mm_region_t *r)
{
  if ((grub_addr_t) ptr & (GRUB_MM_ALIGN - 1))
    grub_fatal ("unaligned pointer %p", ptr);

  for (*r = grub_mm_base; *r; *r = (*r)->next)
    if ((grub_addr_t) ptr > (grub_addr_t) ((*r) + 1)
	&& (grub_addr_t) ptr <= (grub_addr_t) ((*r) + 1) + (*r)->size)
      break;

  if (! *r)
    grub_fatal ("out of range pointer %p", ptr);

  *p = (grub_mm_header_t) ptr - 1;
  if ((*p)->magic == GRUB_MM_FREE_MAGIC)
    grub_fatal ("double free at %p", *p);
  if ((*p)->magic != GRUB_MM_ALLOC_MAGIC)
    grub_fatal ("alloc magic is broken at %p: %lx", *p,
		(unsigned long) (*p)->magic);
}
/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月21日
*
* @brief 将一片内存区域初始化成由一个grub_mm_region_t管理的空闲内存。
*
* @note 注释详细内容:
*
* 本函数实现将一片内存区域初始化成由一个grub_mm_region_t管理的空闲内存的功能。函数首先在
* 该片内存中cast出一片grub_mm_region_t结构，然后再cast出一个grub_mm_header_t结构，之后再
* 将这个grub_mm_region_t区域结构按照由小到大的顺序插入到以grub_mm_base开始的区域链表中。
**/
/* Initialize a region starting from ADDR and whose size is SIZE,
   to use it as free space.  */
void
grub_mm_init_region (void *addr, grub_size_t size)
{
  grub_mm_header_t h;
  grub_mm_region_t r, *p, q;

#if 0
  grub_printf ("Using memory for heap: start=%p, end=%p\n", addr, addr + (unsigned int) size);
#endif

  /* Allocate a region from the head.  */
  r = (grub_mm_region_t) ALIGN_UP ((grub_addr_t) addr, GRUB_MM_ALIGN);
  size -= (char *) r - (char *) addr + sizeof (*r);

  /* If this region is too small, ignore it.  */
  if (size < GRUB_MM_ALIGN)
    return;

  h = (grub_mm_header_t) (r + 1);
  h->next = h;
  h->magic = GRUB_MM_FREE_MAGIC;
  h->size = (size >> GRUB_MM_ALIGN_LOG2);

  r->first = h;
  r->pre_size = (grub_addr_t) r - (grub_addr_t) addr;
  r->size = (h->size << GRUB_MM_ALIGN_LOG2);

  /* Find where to insert this region. Put a smaller one before bigger ones,
     to prevent fragmentation.  */
  for (p = &grub_mm_base, q = *p; q; p = &(q->next), q = *p)
    if (q->size > r->size)
      break;

  *p = r;
  r->next = q;
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月21日
*
* @brief 内存分配的核心函数。
*
* @note 注释详细内容:
*
* 本函数实现内存分配的核心函数的功能。参数first是grub_mm_header_t结构构成的ring buffer的
* 起始地址，参数n是要分配的以GRUB_MM_ALIGN大小为单位的元素个数，参数align为以GRUB_MM_ALIGN
* 大小为单位的对齐要求。函数首先检查first本身是不是已经被分配（GRUB_MM_ALLOC_MAGIC被设置），
* 如果已经被分配那么就表示所有的内存都分配完了，从而返回0；接着在first开始的链表中寻找一个
* 可以用以分配的位置，要求能满足大小，对齐，未分配的要求，并对找到区域进行二次划分；最后
* 标记分配出来的内存区域为GRUB_MM_ALLOC_MAGIC。
**/
/* Allocate the number of units N with the alignment ALIGN from the ring
   buffer starting from *FIRST.  ALIGN must be a power of two. Both N and
   ALIGN are in units of GRUB_MM_ALIGN.  Return a non-NULL if successful,
   otherwise return NULL.  */
static void *
grub_real_malloc (grub_mm_header_t *first, grub_size_t n, grub_size_t align)
{
  grub_mm_header_t p, q;

  /* When everything is allocated side effect is that *first will have alloc
     magic marked, meaning that there is no room in this region.  */
  if ((*first)->magic == GRUB_MM_ALLOC_MAGIC)
    return 0;

  /* Try to search free slot for allocation in this memory region.  */
  for (q = *first, p = q->next; ; q = p, p = p->next)
    {
      grub_off_t extra;

      extra = ((grub_addr_t) (p + 1) >> GRUB_MM_ALIGN_LOG2) % align;
      if (extra)
	extra = align - extra;

      if (! p)
	grub_fatal ("null in the ring");

      if (p->magic != GRUB_MM_FREE_MAGIC)
	grub_fatal ("free magic is broken at %p: 0x%x", p, p->magic);

      if (p->size >= n + extra)
	{
	  extra += (p->size - extra - n) & (~(align - 1));
	  if (extra == 0 && p->size == n)
	    {
	      /* There is no special alignment requirement and memory block
	         is complete match.

	         1. Just mark memory block as allocated and remove it from
	            free list.

	         Result:
	         +---------------+ previous block's next
	         | alloc, size=n |          |
	         +---------------+          v
	       */
	      q->next = p->next;
	    }
	  else if (align == 1 || p->size == n + extra)
	    {
	      /* There might be alignment requirement, when taking it into
	         account memory block fits in.

	         1. Allocate new area at end of memory block.
	         2. Reduce size of available blocks from original node.
	         3. Mark new area as allocated and "remove" it from free
	            list.

	         Result:
	         +---------------+
	         | free, size-=n | next --+
	         +---------------+        |
	         | alloc, size=n |        |
	         +---------------+        v
	       */

	      p->size -= n;
	      p += p->size;
	    }
	  else if (extra == 0)
	    {
	      grub_mm_header_t r;

	      r = p + extra + n;
	      r->magic = GRUB_MM_FREE_MAGIC;
	      r->size = p->size - extra - n;
	      r->next = p->next;
	      q->next = r;

	      if (q == p)
		{
		  q = r;
		  r->next = r;
		}
	    }
	  else
	    {
	      /* There is alignment requirement and there is room in memory
	         block.  Split memory block to three pieces.

	         1. Create new memory block right after section being
	            allocated.  Mark it as free.
	         2. Add new memory block to free chain.
	         3. Mark current memory block having only extra blocks.
	         4. Advance to aligned block and mark that as allocated and
	            "remove" it from free list.

	         Result:
	         +------------------------------+
	         | free, size=extra             | next --+
	         +------------------------------+        |
	         | alloc, size=n                |        |
	         +------------------------------+        |
	         | free, size=orig.size-extra-n | <------+, next --+
	         +------------------------------+                  v
	       */
	      grub_mm_header_t r;

	      r = p + extra + n;
	      r->magic = GRUB_MM_FREE_MAGIC;
	      r->size = p->size - extra - n;
	      r->next = p;

	      p->size = extra;
	      q->next = r;
	      p += extra;
	    }

	  p->magic = GRUB_MM_ALLOC_MAGIC;
	  p->size = n;

	  /* Mark find as a start marker for next allocation to fasten it.
	     This will have side effect of fragmenting memory as small
	     pieces before this will be un-used.  */
	  *first = q;

	  return p + 1;
	}

      /* Search was completed without result.  */
      if (p == *first)
	break;
    }

  return 0;
}
/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月21日
*
* @brief 按对齐要求进行内存分配。
*
* @note 注释详细内容:
*
* 本函数实现按对齐要求进行内存分配的功能。参数align为要求的对齐要求，参数size为要分配的内
* 存大小。从grub_mm_base开始，尝试用grub_real_malloc()来使用该区域第一个grub_mm_header_t
* 进行分配，如果分配成功则直接返回；如果所有的区域都没法分配，则尝试无效磁盘缓冲来增加内
* 存，这是通过调用grub_disk_cache_invalidate_all()来完成的。
**/
/* Allocate SIZE bytes with the alignment ALIGN and return the pointer.  */
void *
grub_memalign (grub_size_t align, grub_size_t size)
{
  grub_mm_region_t r;
  grub_size_t n = ((size + GRUB_MM_ALIGN - 1) >> GRUB_MM_ALIGN_LOG2) + 1;
  int count = 0;

  if (!grub_mm_base)
    goto fail;

  align = (align >> GRUB_MM_ALIGN_LOG2);
  if (align == 0)
    align = 1;

 again:

  for (r = grub_mm_base; r; r = r->next)
    {
      void *p;

      p = grub_real_malloc (&(r->first), n, align);
      if (p)
	return p;
    }

  /* If failed, increase free memory somehow.  */
  switch (count)
    {
    case 0:
      /* Invalidate disk caches.  */
      grub_disk_cache_invalidate_all ();
      count++;
      goto again;

#if 0
    case 1:
      /* Unload unneeded modules.  */
      grub_dl_unload_unneeded ();
      count++;
      goto again;
#endif

    default:
      break;
    }

 fail:
  grub_error (GRUB_ERR_OUT_OF_MEMORY, N_("out of memory"));
  return 0;
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月21日
*
* @brief 无对齐要求进行内存分配。
*
* @note 注释详细内容:
*
* 本函数实现无对齐要求进行内存分配的功能。参数size为要分配的内存大小。实际是调用
* grub_memalign()来实际实现内存分配的。
**/
/* Allocate SIZE bytes and return the pointer.  */
void *
grub_malloc (grub_size_t size)
{
  return grub_memalign (0, size);
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月21日
*
* @brief 无对齐要求进行内存分配并清空分配出来的内存。
*
* @note 注释详细内容:
*
* 本函数实现无对齐要求进行内存分配并清空分配出来的内存的功能。参数size为要分配的内存大小。
* 实际是调用grub_memalign()来实际实现内存分配的，并调用grub_memset()清空分配出来的内存。
**/
/* Allocate SIZE bytes, clear them and return the pointer.  */
void *
grub_zalloc (grub_size_t size)
{
  void *ret;

  ret = grub_memalign (0, size);
  if (ret)
    grub_memset (ret, 0, size);

  return ret;
}

/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月21日
*
* @brief 释放已分配的内存。
*
* @note 注释详细内容:
*
* 本函数实现释放已分配的内存的内存的功能。首先调用get_header_from_pointer()获得该内存的
* grub_mm_header_t和grub_mm_region_t地址；然后将安装两种情况处理：
*
* 1） 如果区域的第一个header已经被分配（GRUB_MM_ALLOC_MAGIC），则将该区域的header设为空
* 闲，并将其设为第一个该区域的第一个header。
* 2） 否则，就在r->first开始的列表中找到恰当的位置，并设置GRUB_MM_FREE_MAGIC，将区域插入
* 空闲列表。
**/
/* Deallocate the pointer PTR.  */
void
grub_free (void *ptr)
{
  grub_mm_header_t p;
  grub_mm_region_t r;

  if (! ptr)
    return;

  get_header_from_pointer (ptr, &p, &r);

  if (r->first->magic == GRUB_MM_ALLOC_MAGIC)
    {
      p->magic = GRUB_MM_FREE_MAGIC;
      r->first = p->next = p;
    }
  else
    {
      grub_mm_header_t q, s;

#if 0
      q = r->first;
      do
	{
	  grub_printf ("%s:%d: q=%p, q->size=0x%x, q->magic=0x%x\n",
		       GRUB_FILE, __LINE__, q, q->size, q->magic);
	  q = q->next;
	}
      while (q != r->first);
#endif

      for (s = r->first, q = s->next; q <= p || q->next >= p; s = q, q = s->next)
	{
	  if (q->magic != GRUB_MM_FREE_MAGIC)
	    grub_fatal ("free magic is broken at %p: 0x%x", q, q->magic);

	  if (q <= q->next && (q > p || q->next < p))
	    break;
	}

      p->magic = GRUB_MM_FREE_MAGIC;
      p->next = q->next;
      q->next = p;

      if (p->next + p->next->size == p)
	{
	  p->magic = 0;

	  p->next->size += p->size;
	  q->next = p->next;
	  p = p->next;
	}

      r->first = q;

      if (q == p + p->size)
	{
	  q->magic = 0;
	  p->size += q->size;
	  if (q == s)
	    s = p;
	  s->next = p;
	  q = s;
	}

      r->first = q;
    }
}
/**
* @attention 本注释得到了"核高基"科技重大专项2012年课题“开源操作系统内核分析和安全性评估
*（课题编号：2012ZX01039-004）”的资助。
*
* @copyright 注释添加单位：清华大学——03任务（Linux内核相关通用基础软件包分析）承担单位
*
* @author 注释添加人员：谢文学
*
* @date 注释添加日期：2013年6月21日
*
* @brief 重新分配内存。
*
* @note 注释详细内容:
*
* 本函数实现重新分配内存的功能。如果参数ptr为空，则直接分配size大小的内存并返回；
* 如果size为0，则直接释放ptr对应的内存；否则如果原本内存所在区域的大小比size要
* 求的更大，则直接返回ptr；否则先重新分配size大小的内存，并将原有内存区域拷贝到
* 新分配的内存中，再释放原有ptr对应的内存，返回新分配的内存。
**/
/* Reallocate SIZE bytes and return the pointer. The contents will be
   the same as that of PTR.  */
void *
grub_realloc (void *ptr, grub_size_t size)
{
  grub_mm_header_t p;
  grub_mm_region_t r;
  void *q;
  grub_size_t n;

  if (! ptr)
    return grub_malloc (size);

  if (! size)
    {
      grub_free (ptr);
      return 0;
    }

  /* FIXME: Not optimal.  */
  n = ((size + GRUB_MM_ALIGN - 1) >> GRUB_MM_ALIGN_LOG2) + 1;
  get_header_from_pointer (ptr, &p, &r);

  if (p->size >= n)
    return ptr;

  q = grub_malloc (size);
  if (! q)
    return q;

  grub_memcpy (q, ptr, size);
  grub_free (ptr);
  return q;
}

#ifdef MM_DEBUG
int grub_mm_debug = 0;

void
grub_mm_dump_free (void)
{
  grub_mm_region_t r;

  for (r = grub_mm_base; r; r = r->next)
    {
      grub_mm_header_t p;

      /* Follow the free list.  */
      p = r->first;
      do
	{
	  if (p->magic != GRUB_MM_FREE_MAGIC)
	    grub_fatal ("free magic is broken at %p: 0x%x", p, p->magic);

	  grub_printf ("F:%p:%u:%p\n",
		       p, (unsigned int) p->size << GRUB_MM_ALIGN_LOG2, p->next);
	  p = p->next;
	}
      while (p != r->first);
    }

  grub_printf ("\n");
}

void
grub_mm_dump (unsigned lineno)
{
  grub_mm_region_t r;

  grub_printf ("called at line %u\n", lineno);
  for (r = grub_mm_base; r; r = r->next)
    {
      grub_mm_header_t p;

      for (p = (grub_mm_header_t) ALIGN_UP ((grub_addr_t) (r + 1),
					    GRUB_MM_ALIGN);
	   (grub_addr_t) p < (grub_addr_t) (r+1) + r->size;
	   p++)
	{
	  switch (p->magic)
	    {
	    case GRUB_MM_FREE_MAGIC:
	      grub_printf ("F:%p:%u:%p\n",
			   p, (unsigned int) p->size << GRUB_MM_ALIGN_LOG2, p->next);
	      break;
	    case GRUB_MM_ALLOC_MAGIC:
	      grub_printf ("A:%p:%u\n", p, (unsigned int) p->size << GRUB_MM_ALIGN_LOG2);
	      break;
	    }
	}
    }

  grub_printf ("\n");
}

void *
grub_debug_malloc (const char *file, int line, grub_size_t size)
{
  void *ptr;

  if (grub_mm_debug)
    grub_printf ("%s:%d: malloc (0x%" PRIxGRUB_SIZE ") = ", file, line, size);
  ptr = grub_malloc (size);
  if (grub_mm_debug)
    grub_printf ("%p\n", ptr);
  return ptr;
}

void *
grub_debug_zalloc (const char *file, int line, grub_size_t size)
{
  void *ptr;

  if (grub_mm_debug)
    grub_printf ("%s:%d: zalloc (0x%" PRIxGRUB_SIZE ") = ", file, line, size);
  ptr = grub_zalloc (size);
  if (grub_mm_debug)
    grub_printf ("%p\n", ptr);
  return ptr;
}

void
grub_debug_free (const char *file, int line, void *ptr)
{
  if (grub_mm_debug)
    grub_printf ("%s:%d: free (%p)\n", file, line, ptr);
  grub_free (ptr);
}

void *
grub_debug_realloc (const char *file, int line, void *ptr, grub_size_t size)
{
  if (grub_mm_debug)
    grub_printf ("%s:%d: realloc (%p, 0x%" PRIxGRUB_SIZE ") = ", file, line, ptr, size);
  ptr = grub_realloc (ptr, size);
  if (grub_mm_debug)
    grub_printf ("%p\n", ptr);
  return ptr;
}

void *
grub_debug_memalign (const char *file, int line, grub_size_t align,
		    grub_size_t size)
{
  void *ptr;

  if (grub_mm_debug)
    grub_printf ("%s:%d: memalign (0x%" PRIxGRUB_SIZE  ", 0x%" PRIxGRUB_SIZE
		 ") = ", file, line, align, size);
  ptr = grub_memalign (align, size);
  if (grub_mm_debug)
    grub_printf ("%p\n", ptr);
  return ptr;
}

#endif /* MM_DEBUG */
