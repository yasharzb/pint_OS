<div dir="rtl">
# تمرین گروهی ۱/۰ - آشنایی با pintos

## شماره گروه:

> نام و آدرس پست الکترونیکی اعضای گروه را در این قسمت بنویسید.

یاشار ظروفچی <yasharzb@chmail.ir>

صبا هاشمی <example@example.com>

امیرمحمد قاسمی <example@example.com>

مهرانه نجفی <example@example.com>

## مقدمات

> اگر نکات اضافه‌ای در مورد تمرین یا برای دستیاران آموزشی دارید در این قسمت بنویسید.

> لطفا در این قسمت تمامی منابعی (غیر از مستندات Pintos، اسلاید‌ها و دیگر منابع درس) را که برای تمرین از آن‌ها استفاده کرده‌اید در این قسمت بنویسید.

# آشنایی با pintos

> در مستند تمرین گروهی ۱۹ سوال مطرح شده است. پاسخ آن ها را در زیر بنویسید.

## یافتن دستور معیوب
</div>


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

با دستور `dumplist &all_list  thread allelem`اطلاعات زیر را می‌بینیم:
<div dir="ltr">

```bash
pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_RUNNING, name = "main", '\000' <repeats 11 times>, stack = 0xc000edec <incomplete sequence \357>, priority = 31, allelem = {prev = 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```
</div>

از اطلاعات بالا متوجه می‌شویم نام ریسه `main` و آدرس آن  `0xc000e000` است.
ریسه‌های دیگر در ادامه دیده می‌شوند:

```bash
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```

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
<div dir="ltr">

```bash
(gdb) info threads
  Id   Target Id         Frame
* 1    Thread <main>     start_process (file_name_=file_name_@entry=0xc0109000) at ../../userprog/process.c:55
```
```bash
(gdb) dumplist &all_list thread allelem
pintos-debug: dumplist #0: 0xc000e000 {tid = 1, status = THREAD_BLOCKED, name = "main", '\000' <repeats 11 times>, stack = 0xc000eeac "\001", priority = 31, allelem = {prev = 0xc0035910 <all_list>, next = 0xc0104020}, elem = {prev = 0xc0037314 <temporary+4>, next = 0xc003731c <temporary+12>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #1: 0xc0104000 {tid = 2, status = THREAD_BLOCKED, name = "idle", '\000' <repeats 11 times>, stack = 0xc0104f34 "", priority = 0, allelem = {prev = 0xc000e020, next = 0xc010a020}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
pintos-debug: dumplist #2: 0xc010a000 {tid = 3, status = THREAD_RUNNING, name = "do-nothing\000\000\000\000\000", stack = 0xc010afd4 "", priority = 31, allelem = {prev = 0xc0104020, next = 0xc0035918 <all_list+8>}, elem = {prev = 0xc0035920 <ready_list>, next = 0xc0035928 <ready_list+8>}, pagedir = 0x0, magic = 3446325067}
```
</div>
۱۰.

خروجی اول مربوط به پیش از اجرای تابع `load` است و خروجی دوم مربوط به بعد آن. همانطور که می‌بینید به صورت خاص `eip` و `esp` تغییر داشته‌اند.

<div dir="ltr">

```bash
{edi = 0x0, esi = 0x0, ebp = 0x0, esp_dummy = 0x0, ebx = 0x0, edx = 0x0, ecx = 0x0, eax = 0x0, gs = 0x23, fs = 0x23, es = 0x23, ds = 0x23, vec_no = 0x0, error_code =
 0x0, frame_pointer = 0x0, eip = 0x0, cs = 0x1b, eflags = 0x202, esp = 0x0, ss = 0x23}

{edi = 0x0, esi = 0x0, ebp = 0x0, esp_dummy = 0x0, ebx = 0x0, edx = 0x0, ecx = 0x0, eax = 0x0, gs = 0x23, fs = 0x23, es = 0x23, ds = 0x23, vec_no = 0x0, error_code =
 0x0, frame_pointer = 0x0, eip = 0x8048754, cs = 0x1b, eflags = 0x202, esp = 0xc0000000, ss = 0x23}
 ```


</div>

۱۱.

۱۲.

۱۳.

## دیباگ

۱۴.

۱۵.

۱۶.

۱۷.

۱۸.

۱۹.