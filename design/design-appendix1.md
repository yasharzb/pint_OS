# Accessing user memory

## Page Directory

A general description about pagedir from document:
> The code in ‘pagedir.c’ is an abstract interface to the 80x86 hardware page table, also called a “page directory” by Intel processor documentation. The page table interface uses a uint32_t * to represent a page table because this is convenient for accessing their internal structure.


Now lets look at *src/userprog/pagedir.c: pagedir_get_page*: (make sure to read the comment)
```c
/* Looks up the physical address that corresponds to user virtual
   address UADDR in PD.  Returns the kernel virtual address
   corresponding to that physical address, or a null pointer if
   UADDR is unmapped. */
void *
pagedir_get_page (uint32_t *pd, const void *uaddr)
{
  uint32_t *pte;

  ASSERT (is_user_vaddr (uaddr));

  pte = lookup_page (pd, uaddr, false);
  if (pte != NULL && (*pte & PTE_P) != 0)
    return pte_get_page (*pte) + pg_ofs (uaddr);
  else
    return NULL;
}
```

If you are a little confused like me this is some explanation from stackoverflow:
>There are three kinds of addresses that the comment is talking about:
>+ Physical address: this is the real address, I.E. the real exact position in the physical memory of your computer.
>+ Kernel virtual address: this is the virtual address at which the kernel sees that physical address.
>+ User virtual address: this is the virtual address at which the user space program sees that physical address.

( and this is some explanation from me:
kernel virtual address and user virtual address to same physical address is (or can be) different )

So I think for accessing user vaddr we need `pagedir_get_page` function. (And I guess that the thing in document about unmapped virtual addresses is about this.)

Before calling this function we can user `is_user_vaddr` in *src/threads/vaddr.h* to check the address is in user space. Also we'll check for null pointers.

And one last thing is this (from doc):
> a 4-byte memory region (like a 32-bit integer) may consist of 2 bytes of valid memory and 2 bytes of invalid memory, if the memory lies on a page boundary

I was thinking about checking the last address too but we don't know the size beforehand cause our pointers are mostly strings and they are null terminated. So Any idea?
