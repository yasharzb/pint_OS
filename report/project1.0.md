<div dir="rtl">
# تمرین گروهی ۱/۰ - آشنایی با pintos

## شماره گروه:

> نام و آدرس پست الکترونیکی اعضای گروه را در این قسمت بنویسید.

یاشار ظروفچی <yasharzb@chmail.ir>

صبا هاشمی <sba.hashemii@gmail.com>

امیرمحمد قاسمی <example@example.com>

مهرانه نجفی <example@example.com>

## مقدمات

> اگر نکات اضافه‌ای در مورد تمرین یا برای دستیاران آموزشی دارید در این قسمت بنویسید.

> لطفا در این قسمت تمامی منابعی (غیر از مستندات Pintos، اسلاید‌ها و دیگر منابع درس) را که برای تمرین از آن‌ها استفاده کرده‌اید در این قسمت بنویسید.

# آشنایی با pintos

> در مستند تمرین گروهی ۱۹ سوال مطرح شده است. پاسخ آن ها را در زیر بنویسید.

## یافتن دستور معیوب


۱.

`‍0xc0000008‍`

۲.

`0x8048757`

۳.

نام تابع: `_start`

دستور: `mov 0x24(%esp), %eax`

۴.

`_start` در فایل  `src/lib/user/entry.c` قرار دارد.

<div dir="ltr">

```c
void
_start (int argc, char *argv[])
{
  exit (main (argc, argv));
}
```

assembly code:

```x86asm
08048754 <_start>:
 8048754:       83 ec 1c                sub    $0x1c,%esp
 8048757:       8b 44 24 24             mov    0x24(%esp),%eax
 804875b:       89 44 24 04             mov    %eax,0x4(%esp)
 804875f:       8b 44 24 20             mov    0x20(%esp),%eax
 8048763:       89 04 24                mov    %eax,(%esp)
 8048766:       e8 35 f9 ff ff          call   80480a0 <main>
 804876b:       89 04 24                mov    %eax,(%esp)
 804876e:       e8 49 1b 00 00          call   804a2bc <exit>
```

</div>
۵ خط اول برای پاس دادن آرگومان‌های تابع هستند.

خط اول با کم کردن استک پوینتر ۲۸ بایت حافظه روی استک `allocate` می‌کند. در خطوط بعدی مقادیر `argv` و `argc` که درون استک قرار داشته‌اند به ترتیب (از راست به چپ) به واسطه‌ی ثبات `eax` در خانه‌های `sp` و `sp+4` قرار می‌گیرند.
در نهایت تابع `main` برنامه‌ی کاربر `(do-nothing)` صدا زده می‌شود.

دستور `call` آدرس بازگشت را در استک ذخیره می‌کند و به لیبل مورد نظر پرش می‌کند.
خروجی توابع در ثبات `eax` ریخته می‌شود. در خط ۷ خروجی `main` درون استک ریخته می‌شود و در نهایت `exit` صدا زده می‌شود.

۵.

این دستور با این فرض که `argc` و `argv` به عنوان آرگومان‌های تابع `_start` درون استک قرار دارد می‌خواهد `argv` را از استک درون ثبات `eax` بریزد اما این آرگومان‌ها در استک قرار داده نشده‌اند و در نتیجه آدرس خانه‌‌ای که می‌خواهد خوانده شود برابر مقدار دستور ۱ می‌شود. (در `do-nothing.result` می‌بینیم که مقدار `esp` در هنگام رخ دادن خطا برابر  `0xbfffffe4` است که اگر با `0x24` جمع شود مقدار آن همان `0xc000008` می‌شود). 
محدوده مقادیر حافظه‌ی مجازی که برنامه‌ی کاربر می‌تواند به آن دسترسی داشته باشد از 0 تا `PHYS_BASE` است که مقدار `PHYS_BASE` به طور دیفالت برابر `0xc000000` است در نتیجه برنامه به آدرس ذکر شده دسترسی ندارد و دچار خطا می‌شود.

## به سوی crash

۶.

با دستور
`dumplist &all_list  thread allelem`
اطلاعات زیر را می‌بینیم:

<div dir="ltr">

```bash
pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_RUNNING, name = "main", '\000' <repeats 11 times>, stack = 0xc000edec <incomplete sequence \357>, priority = 31, allelem = {prev = 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```

</div>

از اطلاعات بالا متوجه می‌شویم نام ریسه `main` و آدرس آن  `0xc000e000` است.
ریسه‌های دیگر در ادامه دیده می‌شوند:

<div dir="ltr">


```bash
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```
</div>


۷.

یا دستور `thread apply all backtrace` اطلاعات `backtrace` ریسه‌ی کنونی دیده می‌شود:
<div dir="ltr">

```bash
#0  process_execute (file_name=file_name@entry=0xc0007d50 "do-nothing") at ../../userprog/process.c:36ready_list+8>}, pagedir = 0x0, magic = 3446325067}
#1  0xc0020268 in run_task (argv=0xc00357cc <argv+12>) at ../../threads/init.c:288
#2  0xc0020921 in run_actions (argv=0xc00357cc <argv+12>) at ../../threads/init.c:340
#3  main () at ../../threads/init.c:133e@entry=0xc0007d50 "do-nothing") at ../../userprog/process.c:36
```
</div>

خط ۲۸۸ در پرونده‌ی با کد  `process_wait (process_execute (task))` تابع `process_execute` را صدا می‌زند.

خط ۳۴۰ در پرونده `init.c` با کد `a->function (argv)` تابع `run_task` را صدا می‌زند.

خط ۱۳۳ در پرونده‌ی `init.c` با کد `run_actions (argv)` تابع `run_actions` را صدا می‌زند.

۸.

با دستور `break start_process` کار را شروع می‌کنیم و با ادامه دادن کد به این تابع می‌رویم.

<div dir="ltr">

```bash
pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_BLOCKED, name = "main", '\000' <repeats 11 times>, stack = 0xc000eeac "\001", priority = 31, allelem = {prev = 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0037314 <temporary+4>, next = 0xc003731c <temporary+12>}, pagedir = 0x0, magic = 3446325067}

pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc010a020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}

pintos-debug: dumplist #2: 0xc010a000 {tid = 3, status = THREAD_RUNNING, name = "do-nothing\000\000\000\000\000", stack = 0xc010afd4 "", priority = 31, allelem = {prev = 0xc0104020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```
</div>

۹.

این ترد در ترد `main` که در پاسخ سوال قسمت قبل قابل مشاهده است ساخته می‌شود.
کد مربوط به ساخته شدن این ترد در تابع
`process_execute`
(خط ۴۵)
وجود دارد و به صورت زیر است:

<div dir="ltr">

```c
tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
```

</div>

همانطور که در خروجی `dumplist`
در پاسخ قسمت قبل می‌بینید یک ریسه‌ی جدید با نام `do-nothing` که همان آرگومان `file_name` است ساخته شده است.

۱۰.

خروجی اول مربوط به پیش از اجرای تابع `load` است و خروجی دوم مربوط به بعد آن. همانطور که می‌بینید به صورت خاص `eip` و `esp`که stack pointer و instruction pointer هستند، تغییر داشته‌اند.

<div dir="ltr">

```bash
{edi = 0x0, esi = 0x0, ebp = 0x0, esp_dummy = 0x0, ebx = 0x0, edx = 0x0, ecx = 0x0, eax = 0x0, gs = 0x23, fs = 0x23, es = 0x23, ds = 0x23, vec_no = 0x0, error_code =
 0x0, frame_pointer = 0x0, eip = 0x0, cs = 0x1b, eflags = 0x202, esp = 0x0, ss = 0x23}

{edi = 0x0, esi = 0x0, ebp = 0x0, esp_dummy = 0x0, ebx = 0x0, edx = 0x0, ecx = 0x0, eax = 0x0, gs = 0x23, fs = 0x23, es = 0x23, ds = 0x23, vec_no = 0x0, error_code =
 0x0, frame_pointer = 0x0, eip = 0x8048754, cs = 0x1b, eflags = 0x202, esp = 0xc0000000, ss = 0x23}
 ```

</div>

۱۱.

در این قسمت وقتی به `iret` رسیدیم متوجه می‌شیم که هنگام جلو رفتن اینگونه برخورد می‌کند:

<div dir="ltr">

```bash
(gdb) n
0x08048754 in ?? ()
(gdb) n
Cannot find bounds of current function
```
</div>

اگر با گام‌های کوچکتر پیش رویم خواهیم داشت:

<div dir="ltr">

```bash
(gdb) ni
0x08048757 in ?? ()
(gdb) ni
pintos-debug: a page fault exception occurred in user mode
pintos-debug: hit 'c' to continue, or 's' to step to intr_handler
0xc0021b95 in intr0e_stub ()
```

</div>

اگر نیک بنگریم، آدرس گفته شده در حقیقت همان `%eip` است که در بند ۱۰ تغییرش را دیدیم و همان آدرس مجازی که موجب crash شد.
همچنین طبق سطر اول خطا درمی‌یابیم که در userspace هستیم و این خطا همان خطایی است که در ابتدای کار با آن مواجه شدیم.

هم چنین اگر به مقادیر رجیسترها هنگام رسیدن به دستور `iret` توجه کنیم
می‌بینیم که این مقادیر مربوط به کرنل هستند:

<div dir="ltr">

```bash
(gdb) where
#0  intr_exit () at ../../threads/intr-stubs.S:64
(gdb) info registers
eax            0x0      0
ecx            0x0      0
edx            0x0      0
ebx            0x0      0
esp            0xc010af94       0xc010af94
ebp            0x0      0x0
esi            0x0      0
edi            0x0      0
eip            0xc0021b19       0xc0021b19 <intr_exit+10>
eflags         0x292    [ AF SF IF ] 
cs             0x8      8
ss             0x10     16
ds             0x23     35
es             0x23     35
fs             0x23     35
gs             0x23     35
```

</div>

پس از جلو رفتن به اندازه‌ی یک استپ (si) خواهیم دید که این مقادیر اکنون برابر مقادیری که در if_ داشتیم هستند و وارد userspace شده‌ایم:

<div dir="ltr">

```bash
(gdb) where
#0  0x08048754 in ?? ()
(gdb) info registers 
eax            0x0      0
ecx            0x0      0
edx            0x0      0
ebx            0x0      0
esp            0xc0000000       0xc0000000
ebp            0x0      0x0
esi            0x0      0
edi            0x0      0
eip            0x8048754        0x8048754
eflags         0x202    [ IF ]
cs             0x1b     27
ss             0x23     35
ds             0x23     35
es             0x23     35
fs             0x23     35
gs             0x23     35
(gdb) 
```

</div>


در کامنت‌های کد `start_process` قبل از دستور `asm volatile` گفته شده:
<div dir="ltr">

> Start the user process by simulating a return from an interrupt, implemented by intr_exit (in threads/intr-stubs.S).  Because intr_exit takes all of its arguments on the stack in the form of a `struct intr_frame', we just point the stack pointer (%esp) to our stack frameand jump to it. 
</div>

هم چنین در داکیومنت دستور `iret` داریم:

<div dir="ltr">

> Returns program control from an exception or interrupt handler to a program or procedure that was interrupted by an exception, an external interrupt, or a software-generated interrupt
</div>

حال چون در ابتدای دستور `asm volatile` فریم مربوط به برنامه‌ی کاربر را در استک می‌ریزیم پس تابع `iter` 
به userspace باز می‌گردد
و برنامه‌ی کاربر شروع به اجرا می‌کند.


۱۲.
<div dir="ltr">

```bash
(gdb) info registers 
eax            0x0      0
ecx            0x0      0
edx            0x0      0
ebx            0x0      0
esp            0xc0000000       0xc0000000
ebp            0x0      0x0
esi            0x0      0
edi            0x0      0
eip            0x8048754        0x8048754
eflags         0x202    [ IF ]
cs             0x1b     27
ss             0x23     35
ds             0x23     35
es             0x23     35
fs             0x23     35
gs             0x23     35
```
</div>

مقادیر `esp` و `eip` همان مقادیر `if_` هستند.

۱۳.

<div dir="ltr">

```bash
(gdb) bt
#0  0xc0021b95 in intr0e_stub ()
(gdb) btpagefault
#0  _start (argc=<unavailable>, argv=<unavailable>) at ../../lib/user/entry.c:9
```
</div>

## دیباگ

۱۴.

همانطور که در بند ۵ دیدیم، مشکل اصلی این است که آرگومان‌ها از جایی بیرون از فضای کاربر خواند می‌شوند و این موجب `page fault` می‌شود. برای رفع این مشکل می‌توانیم `if_`ی که در بند ۱۰ صحبتش بود را طوری تنظیم کنیم که `esp` برای آن به آدرس داخلی‌تری از پشته نگاشت شود که پس از اضافه شدن جمعا ۸ با آدرس باز هم درون فضای کاربر بیفتد.
در این راستا، تابع `load` در فایل `process.c` را بررسی می‌کنیم. در خط ۳۰۰ این فایل با عبارت زیر مواجه می‌شویم:

<div dir="ltr">

```c
  /* Set up stack. */
  if (!setup_stack(esp))
    goto done;
```

</div>
 
 پس تابع `setup_stack` را بررسی می‌کنیم. در خط ۴۳۴ داریم:

 <div dir="ltr">

 ```c
     success = install_page(((uint8_t *)(PHYS_BASE)) - PGSIZE, kpage, true);
    if (success)
      *esp = PHYS_BASE;
 ```

 </div>

اگر `esp` را به اندازه‌ی ۱۶ خانه به عقب بیاوریم، نتیجتا به نیازی به دسترسی به آدرسی بیشتر از `0xc000000` که کران گفته‌ی فضای کاربر است نخواهد داشت. پس خط آخر را بدین سان تغییر می‌دهیم:

<div dir="ltr">

```c
      *esp = PHYS_BASE - 0x00000010;
```
</div>

با انجام این تغییرات و اعمال مجدد دستورهای `make` و `make check` نتیجه‌ی زیر در فایل `do-nothing.result` مطلوب است:

<div dir="ltr">

```bash
PASS
```
</div>

۱۵.

طبق do-stack-align.ck باید مقدار ۱۲ را خروجی بگیریم.

<div dir="ltr">
do-stack-align: exit(12)‍
</div>

اما ما خروجی ۰ میگرفتیم. پس کافی است کاری کنیم که باقیمانده `esp%` به ۱۶ برابر ۱۲ شود. پس تغییر زیر را اعمال می‌کنیم.

<div dir="ltr">

```c
      *esp = PHYS_BASE - 0x00000014;
```

</div>

۱۶.

مقدار `esp%` برابر است با `0xfffd40c` و دو مقدار روبه‌روی آن، دو کلمه‌ی بالای استک است.
 <div dir="ltr">

```bash
(gdb) x/2xw $esp
0xffffd40c:     0x00000001      0x000000a2
```

 </div>


۱۷.

`arg[0]` برابر ۱ و `arg[1]` برابر ۱۶۲ (0xa2) است که
همان مقادیری‌ست که در قسمت قبل گرفتیم.

۱۸.

<div dir="ltr">

```bash
(gdb) break sema_down
Breakpoint 2 at 0xc00226fc: file ../../threads/synch.c, line 62.
(gdb) c
Continuing.

Breakpoint 2, sema_down (sema=sema@entry=0xc0036efc <descs+316>) at ../../threads/synch.c:62
(gdb) backtrace 
#0  sema_down (sema=sema@entry=0xc0036efc <descs+316>) at ../../threads/synch.c:62
#1  0xc0022a18 in lock_acquire (lock=lock@entry=0xc0036ef8 <descs+312>) at ../../threads/synch.c:199
#2  0xc002388f in free (p=<optimized out>, p@entry=0xc010840c) at ../../threads/malloc.c:236
#3  0xc002cde7 in inode_close (inode=0xc010840c) at ../../filesys/inode.c:184
#4  0xc002c54f in file_close (file=file@entry=0xc010705c) at ../../filesys/file.c:51
#5  0xc002afe1 in load (esp=0xc010afa0, eip=0xc010af94, file_name=<optimized out>) at ../../userprog/process.c:320
#6  start_process (file_name_=<optimized out>, file_name_@entry=0xc0109000) at ../../userprog/process.c:65
#7  0xc002132f in kernel_thread (function=0xc002ab3c <start_process>, aux=0xc0109000) at ../../threads/thread.c:424
#8  0x00000000 in ?? ()
```

</div>

بر روی تابع `sema_down` یک breakpoint می‌گذاریم. همانطور که معلوم است اجرای برنامه ابتدا به `load` در `start_process` می‌رسد و سپس به `file_close`  و `indoe_close` و `free` و `lock_acquire` و درانتها به `sema_down` می‌رسد.




TODO suggestion:

`sema_down(&temporary)` در تابع `process_wait` در فایل process.c قرار دارد.

کاربرد این
semaphore
این است که پس از اتمام اجرای ترد، ترد دیگری که منتظر اتمام اجرای آن است (و در `process_wait` منتظر است) متوجه شود و از حالت blocked خارج شود.


۱۹.

<div dir="ltr">

```bash
(gdb) dumplist &all_list thread allelem 

pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_BLOCKED, name = "main", '\000' <repeats 11 times>, stack = 0xc000eeac "\001", priority = 31, allelem = {prev = 0xc0035910 <all_list>, next= 0xc0104020}, elem = {prev = 0xc0037314 <temporary+4>, next = 0xc003731c <temporary+12>}, pagedir = 0x0, magic = 3446325067}

pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc010a020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}

pintos-debug: dumplist #2: 0xc010a000 {tid = 3, status = THREAD_RUNNING, name = "do-nothing\000\000\000\000\000", stack = 0xc010ad04 "8\255\020\300\365Q\002\300\210p\003\300\001", priority = 31, allelem= {prev = 0xc0104020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0xc010b000, magic = 3446325067}
```

</div>

در این حالت ۳ ریسه داریم که ریسه do-nothing ریسه‌ای است که این تابع را دارد اجرا می‌کند.

TODO suggestion:

ترد main این تابع را اجرا می‌کند که در آدرس `0xc000e000` قرار دارد.
در این حالت می‌بینیم که ترد main در استیت THREAD_RUNNING است و ترد do-nothing در استیت THREAD_READY که یعنی در حال حاضر اجرا نمی‌شود ولی آماده‌ی اجرا است و در هنگام اجرای بعدی scheduler می‌تواند اجرا شود.
<div dir="ltr">

```bash
(gdb) b process.c:84
Breakpoint 1 at 0xc002aacc: file ../../userprog/process.c, line 84.
(gdb) c
Continuing.

Breakpoint 1, process_wait (child_tid=3) at ../../userprog/process.c:93
(gdb) dumplist &all_list thread allelem

pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_RUNNING, name = "main", '\000' <repeats 11 times>, stack = 0xc000edec "\375\003", priority = 31, allelem = {prev =
 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}

pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e0
20, next = 0xc010a020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
p
intos-debug: dumplist #2: 0xc010a000 {tid = 3, status = THREAD_READY, name = "do-nothing\000\000\000\000\000", stack = 0xc010afd4 "", priority = 31, allelem = {prev = 0xc0104020
, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```

</div>

</div>
