# Accessing user memory
<div dir="rtl">

+ توضیح دهید خواندن و نوشتن داده‌های کاربر از داخل هسته، در کد شما چگونه انجام شده است.

+ به چه دلیل روش دسترسی به حافظه سطح کاربر از داخل هسته را این‌گونه پیاده‌سازی کرده‌اید؟

</div>

#

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


# Exec and wait
<div dir="rtl">

+ پیاده‌سازی فراخوانی سیستمی wait را توضیح دهید و بگویید چگونه با پایان یافتن پردازه در ارتباط است.

+ فراخوانی سیستمی exec نباید قبل از پایان بارگذاری فایل اجرایی برگردد، چون در صورتی که بارگذاری فایل اجرایی با خطا مواجه شود باید -۱ برگرداند. کد شما چگونه از این موضوع اطمینان حاصل می‌کند؟ چگونه وضعیت موفقیت یا شکست در اجرا به ریسه‌ای که exec را فراخوانی کرده اطلاع داده می‌شود؟

+ پردازه‌ی والد P و پردازه‌ی فرزند C را درنظر بگیرید. هنگامی که P فراخوانی wait(C) را اجرا می‌کند و C هنوز خارج نشده است، توضیح دهید که چگونه همگام‌سازی مناسب را برای جلوگیری از ایجاد شرایط مسابقه (race condition) پیاده‌سازی کرده‌اید. وقتی که C از قبل خارج شده باشد چطور؟ در هر حالت چگونه از آزاد شدن تمامی منابع اطمینان حاصل می‌کنید؟ اگر P بدون منتظر ماندن، قبل از C خارج شود چطور؟ اگر بدون منتظر ماندن بعد از C خارج شود چطور؟ آیا حالت‌های خاصی وجود دارد؟
</div>

#

First have a look at these system calls explanation in document (Berekly project1 doc page 17-18)


We need a way to know if the process called `wait(pid)` is the parent of pid. I think we can add a `parent_tid` (or pid!) in thread struct in *thread.h* then in wait we can check if `parent_tid` of child process is same as currect thread. Also we need to store a list of all threads since we need return value of child thread in `wait` syscall even if it has already exited. Currectly there is a `all_list` in *thread.c* but it remove threads when they exit. 


For implementing exec can't we simply use `process_execute`?
