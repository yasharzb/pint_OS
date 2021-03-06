<div dir="rtl">

# تمرین گروهی ۱.۱ - گزارش نهایی

گروه
-----

یاشار ظروفچی <yasharzb@chmail.ir>

صبا هاشمی <sba.hashemii@gmail.com> 

امیرمحمد قاسمی <ghasemiamirmohammad@yahoo.com> 

مهرانه نجفی <najafi.mehraneh@gmail.com> 



پاس‌دادن آرگومان
============

به صورت کلی میتوان این بخش را به دو زیر بخش تقسیم کرد

* جداسازی ورودی‌ها و آپشن‌های هر دستور خط‌فرمان
* قرار دادن آرگومان ها در پشته
  
برای بخش نخست باید در پرونده `process.c` در تابع `load` تغییراتی ایجاد میکردیم. زیرا دقیقا در این تابع است که پرونده اجرایی جدید بارگذاری میشود و لازم است که نام پرونده اجرایی را از آرگومان‌های ورودی جدا کنیم. همانطور که در بخش طراحی مطرح کرده بودیم، از تابع strtok_r استفاده کردیم.

برای قسمت دوم، تابع جدید `setup_stack` را با امضا (signature) زیر به جای تابع قبلی استفاده کردیم.

<div dir="ltr">

```c
bool setup_stack(int argc, char **argv, void **esp)
```
</div>

 در این تابع مانند تابع `s‍‍‍‍‍etup_stack` پیشین، یک صفحه حافظه متناظر با پشته کاربر ایجاد میکنیم و آرگومان ها را در آن قرار میدهیم. همچنین اندازه پشته را با اندازه صفحه حافظه مقایسه میکنیم و اگر صفحه به اندازه کافی فضا برای قرار دادن آرگومان ها نداشت، مقدار false را برمیگردانیم. 
 
 طول پشته کاربر با استفاده از طول آرگومان‌ها و تعدادشان وهمچنین تعداد بایت‌های لازم برای تراز کردن پشته کاربر مشخص میشود. دو مرحله ترازسازی پشته کاربر نیاز داریم. در مرحله اول باید اطمینان حاصل کنیم که تعدادی بایت خالی پس از درج آرگومان‌ها به پشته اضافه کنیم که در مجموع مضربی از ۴ برای رشته‌ها استفاده کرده باشیم. در مرحله دوم باید اطمینان حاصل کنیم در نهایت اشاره‌گر پشته قبل از اضافه کردن `return address` به پشته در مضربی از ۱۶ قرار میگیرد. پس مقدار فضای خالی مورد نظر به این منظور را نیز حساب می‌کنیم و قبل از قرار دادن آرگومان‌ها درون حافظه به این مقدار فضای خالی در نظر می‌گیریم.

در نهایت لازم است به این نکته اشاره کنیم که هر صفحه‌ای از حافظه که با تابع `palloc_get_page` گرفته شده در صورت بروز خطا (مثلا جا نشدن آرگومان ها در استک) و یا به اتمام رسیدن کار (مثلا صفحه‌های استفاده شده برای تجزیه‌ی آرگومان‌ها) آزاد می‌شود.


فراخوانی‌های سیستمی
================

برای فراخوانی های سیستمی بر روی پردازه‌ها،‌ اعم از exit, wait, exec, halt کارهای زیر انجام شده‌است.
مطابق مستند طراحی، اعضای  زیر را به ساختار thread اضافه کرده‌ایم.

یک `struct file *executable_file` نیز اضافه شده که در مستند طراحی ذکر نشده بود. توضیحات آن در ادامه موجود است.
<div dir="ltr">

```c
struct thread {
    ...

    tid_t parent_tid;

    struct semaphore exited;
    int exit_value;
    bool wait_on_called;
    struct semaphore can_free;    

    struct list children_list;
    struct list_elem child_elem;

    bool load_success_status;
    struct semaphore load_done;

    struct file *executable_file;
    
    int fd_counter;
    struct list fd_list;
}
```
</div>


این اعضا در تابع `thread_create` مقداره دهی اولیه میشوند. پدر ریسه‌ای که در حال ایجاد شدن در تابع `thread_create` است  همان ریسه‌ای هست که دارد این تابع را اجرا می‌کند. پس پدر یک ریسه بدین صورت و با تابع `thread_current` مشخص میشود و ریسه‌ی جدید در لیست فرزندان ریسه‌ی پدر درج می‌شود.

برای پیاده سازی  exec، تابعی با همین نام در `syscall.c` اضافه کرده‌ایم که مسئولیت دارد که آرگومان ورودی به آن را عنوان یک دستور  اجرا کند. به همین منظور این تابع در ابتدا `process_execute` را با آرگومان ورودی‌اش صدا میکند. در این جا میدانیم که خروجی  `process_execute` یک ‍`tid_t` است. پس برای دسترسی به خود ساختار ریسه تابع `get_thread` در `thread.c` اضافه شده که یک ورودی از جنس `tid_t` میگیرد و ریسه‌ای با این آیدی را خروجی می‌دهد. چون لازم است که ابتدا پرونده اجرایی جدید بارگذاری شود و بعد به برنامه کاربر برگردیم، در این جا سمافور `load_done` فرزند را کاهش می‌دهیم.  این سمافور در تابع `start_process` بعد از این که از نتیجه بارگذاری خبردار شدیم، افزایش میباید و بدین ترتیب تابع exec از وضعیت بارگذاری پرونده اجرایی فرزند آگاه می‌شود و به برنامه کابر نتیجه را اعلام می‌کند.

برای پیاده سازی wait، تابع `process_wait` در `process.c‍‍` تکمیل شد که فرآیند آن دقیقا مشابه مستند طراحی است. (در بخش همگام سازی توضیحات آن وجود دارد.) به طور خلاصه از سمافور ‍`exited` برای اطلاع به ریسه پدر هنگام خروجی ریسه فرزند استفاده می‌شود. این سمافور در `process_wait‍` کاهش می‌باید. بدین صورت پدر از مقدار خروجی ریسه فرزند آگاه می‌شود و این جاست که ریسه پدر سمافور `can_free` را افزایش می‌دهد و فرزند فرآیند خروج خود را طی می‌کند. بررسی هایی از این دست که ورودی تابع `process_wait‍‍` معتبر باشد و دوبار بر روی ریسه فرزند توفق نکرده باشیم نیز انجام میشوند. 
همچنین هنگام خروج یک ریسه، سمافور `can_free` برای تمام فرزندان آن ریسه افزایش می‌باید.

\* ورودی‌های این توابع در صورتی که پوینتر باشند، پیش ازینکه به توابع مربوط به خود بروند، در  پرونده‌ی syscall.c توسط تابع `get_kernel_va_for_user_pointer` سنجیده می‌شوند که پوینتر صحیحی باشند و پس از کپی شدن این آدرس از حافظه‌ی کاربر به حافظه‌ی کرنل، آدرس صحیح اختصاص یافته به این پوینتر در حافظه‌ی کرنل را برمی‌گرداند.


فراخوانی سیستمی روی پرونده‌ها
============================

پرونده‌ای به نام `file-descriptor` در آدرس `pintos/src/filesys` قرار گرفته. مدیریت `file descriptor`ها در این پرونده انجام می‌شود.


<div dir="ltr">

```c
typedef struct file_descriptor
{
   struct file *file;
   char *file_name;
   int fd;
   struct list_elem fd_elem;
} file_descriptor;

```
</div>

استراکت `file_descriptor` فایل اصلی‌ای که این `file descriptor` قرار است به آن اشاره کند و نام این `file` را نگه‌داری می‌کند. 

همچنین چون هر `file descriptor` یک عدد دارد (در واقع هر `file descriptor` تنها یک عدد است)، عدد آن را در `int fd` ذخیره می‌کنیم. 

`fd_elem` برای پیدا کردن یک `file descriptor` مشخص در لیست `fd_list` در یک  `thread` به کار می‌رود.
________________
در ادامه توابع کلی در این پرونده را به اختصار توضیح می‌دهیم:

### `void file_descriptor_init`
در این پرونده برای مدیریت اعمال `file descriptor` ها دو قفل گلوبال داریم. این تابع مقادیر اولیه این دو قفل را قرار می‌دهد. در ابتدا هیچکدام از این دو قفل قرار نیست فعال باشند.

### `static int allocate_fd_number(void)`
این تابع به یک `file descriptor` مخصوص یک ترد یک عدد اختصاص می‌دهد. برای اینکه این عملیات به مکانی برای `race condition` تبدیل نشود، هر زمانی که یک عدد بخواهد اختصاص یابد قفل `fd_number_lock` پیش از شروع کار فعال می‌شود و پس از اختصاص دادن یک عدد یکتا به یک `file descriptor` قفل دوباره غیر فعال می‌شود.

### `int is_valid_fd(int fd)`
بررسی می‌کند تعداد `file descriptor`های یک ترد از ماکسیمم تعداد مجاز (۱۲۸) بیشتر نشود. همچنین بررسی می‌کند که `file descriptor` مقداری کمتر از ۲ نداشته باشد زیرا این مقادیر برای `stdout` و `stdin` ذخیره شده‌ اند.

### `file_descriptor * get_file_from_current_thread(int fd)`
این تابع با گرفتن یک عدد به عنوان ورودی، در ترد فعلی `file descriptor` ای که این عدد را به عنوان fd در استراکت خود ذخیره کرده خروجی می‌دهد. درصورتی که چنین `file descriptor` ای وجود نداشت `NULL` برمی‌گرداند.
_________________

در این بخش به نحوه‌ی پیاده‌سازی هر کدام از فراخوانی‌های فایلی خواسته شده می‌پردازیم.

### `create`
برای پیاده سازی این فراخوانی اول قفل گلوبال wr_lock را فعال می‌کنیم تا به هیچ‌عنوان `race condition` پیش نیاید. سپس تابع `filesys_create` را صدا می‌کنیم. این تابع یک فایل با نام مورد نظر ما می‌سازد و در صورتی که چنین فایلی از پیش وجود داشت یا مشکلاتی به وجود آمد که نهایتا چنین فایلی ساخته نشد false برمی‌گرداند.

پس از انجام این فرآیند قفل `wr_lock` را آزاد می‌کنیم.

### `remove`
برای پیاده سازی این فراخوانی اول قفل گلوبال `wr_lock` را فعال می‌کنیم تا به هیچ‌عنوان `race condition` پیش نیاید. سپس تابع `filesys_remove `را صدا می‌کنیم. این تابع یک فایل با نام مورد نظر ما را حذف می‌کند و در صورتی که چنین فایلی از پیش وجود نداشت یا مشکلاتی به وجود آمد که نهایتا فایل مورد نظر حذف نشد `false` برمی‌گرداند.

اینجا نباید `file descriptor`های مربوط به فایلی که حذف شده را از بین ببریم. تمام `file descriptor`هایی که به این فایل اشاره می‌کردند همچنان باید بتوانند به کار خود ادامه دهند.

پس از انجام این فرآیند قفل `wr_lock` را آزاد می‌کنیم.

### `open`

این فراخوانی در حالت `SYS_OPEN` در `syscall.c` مدیریت خواهد شد. پس از بررسی معتبر بودن نشانی `args[1]` و `NULL` نبودن آن یک `file_descriptor` ایجاد می‌شود و سپس مقدار `fd` در `eax` قرار داده می‌شود.
عملکرد `create_file_descriptor` به این صورت است که پس از اخذ `rw_lock` و با کمک تابع داخلی `filesys_open` فایل خواسته شده را باز می‌کند و مقادیر `fd` ، `file_name` و `file` را برای یک `file_descriptor` جدید قرار می‌دهد. سپس آن را به `fd_list` ترد اضافه می‌کند و نهایتا قفل را آزاد می‌کند.

### `close`

این فراخوانی در حالت `SYS_CLOSE` در `syscall.c` مدیریت خواهد شد. که تابع `close_fd` را صدا می‌زند.
تابع گفته شده پس از اخذ `rw_lock` ابتدا با کمک `get_file_from_current_thread`  از `fd_list` موجود در ترد، `fd` مربوطه و فایل متناظر با آن را پیدا می‌کند. سپس با تابع داخلی `file_close` آن را می‌بندد و متعاقبا آن را از `fd_list` ترد حذف می‌کند. در نهایت پس از آزادسازی فضای `fd` و آزاد کردن قفل عملیات به پایان می‌رسد.

### `write`

این فراخوانی در حالت `SYS_WRITE` در `syscall.c` مدیریت خواهد شد. پس از بررسی معتبر بودن نشانی `args[1]` و `NULL` نبودن آن سه حالت چک خواهد شد.

* `stdin`: در این مرحله سناریویی برای آن وجود ندارد.
* `stdout`: با کمک تابع `putbuf` مقدار آن نوشته می‌شود.
* یک فایل خارجی: تابع `fd_write` صدا زده می‌شود.

تابع `fd_write` پس از اخذ `rw_lock` و بررسی معتبر بودن `fd` آن فایل مربوطه به آن از `fd_list` دریافت می‌شود و سپس با تابع داخلی `file_write` ورودی با اندازه‌ی مشخص شده نوشته می‌شود و نهایتا قفل آزاد می‌گردد. همچنین در `eax` میزان بایت نوشته شده نیز قرار می‌گیرد.

### `read`

این فراخوانی در حالت `SYS_READ` در `syscall.c` مدیریت خواهد شد. 
ابتدا بررسی می‌شود که `args[2]` که کاربر به عنوان بافر داده است به اندازه‌ی `args[3]` که سایز داده شده برای خواندن است یک حافظه‌ی معتبر در فضای کاربر است یا نه.
پس از آن سه حالت زیر چک خواهد شد:

* `stdout`: در این مرحله سناریویی برای آن وجود ندارد.
* `stdout`: با تابع `input_getc` از کاربر ورودی می‌گیرد.
* یک فایل خارجی: تابع `fd_read` صدا زده می‌شود.

تابع `fd_read` پس از اخذ `rw_lock` و بررسی معتبر بودن `fd` آن فایل مربوطه به آن از `fd_list` دریافت می‌شود و سپس با تابع داخلی `file_read` محتوا خوانده می‌شود و نهایتا قفل آزاد می‌گردد. همچنین در `eax` میزان بایت خوانده شده نیز قرار می‌گیرد.

### `filesize`
اول قفل `wr_lock` را فعال می‌کنیم تا از `race condition` ناخواسته جلوگیری شود. با گرفتن یک عدد به عنوان fd سعی می‌کنیم `file descriptor` مربوط به این عدد را در این ترد بیابیم. پس از پیدا کردن این فایل، با صدا زدن تابع `file_length` سایز این فایل را برمی‌گردانیم. در صورت پیدا نشدن چنین `file descriptor`ای، ۱- برمی‌گردانیم.
در آخر قفل wr_lock را آزاد می‌کنیم.

### `seek`
اول قفل `wr_lock` را فعال می‌کنیم تا از `race condition` ناخواسته جلوگیری شود. با گرفتن یک عدد به عنوان fd سعی می‌کنیم `file descriptor` مربوط به این عدد را در این ترد بیابیم. پس از پیدا کردن این فایل، با صدا زدن تابع `file_seek` به جایی از فایل که توسط position مشخص شده خواهیم رفت. این تابع خروجی ندارد.
در آخر قفل `wr_lock` را آزاد می‌کنیم.

### `tell`

اول قفل `wr_lock` را فعال می‌کنیم تا از `race condition` ناخواسته جلوگیری شود. با گرفتن یک عدد به عنوان fd سعی می‌کنیم `file descriptor` مربوط به این عدد را در این ترد بیابیم. پس از پیدا کردن این فایل، با صدا زدن تابع `file_seek` مکان بایت بعدی‌ای که از فایل می‌خواهیم بخوانیم یا در آن بنویسیم را به صورت بایت به ما برمی‌گرداند.
در آخر قفل `wr_lock `را آزاد می‌کنیم.

## آزاد کردن فضای مربوط به توصیف‌کننده‌های فایل
فضای گرفته شده برای توصیف‌کننده‌های فایلی که تابع `close` روی آن‌ها صدا زده می‌شود پس از بسته شدن فایل آزاد می‌شود. سایر فایل‌هایی که در لیست توصیف‌کننده‌های فایل  باقی می‌مانند در تابع `thread_exit` بسته می‌شوند و فضای مربوط به آن‌ها آزاد می‌شود.


## جلوگیری از نوشتن روی فایل اجرایی
فایل اجرایی یک پراسس در تابع `load` در `process.c` باز می‌شود.
در آن‌جا تابع `file_deny_write` را صدا می‌زنیم تا از نوشتن روی فایل جلوگیری شود.
چون در صورت بستن این فایل تابع `file_allow_write` صدا زده می‌شود پس لازم است که فایل را تا انتهای پراسس باز نگه داریم. به همین منظور فیلد `executable_file` در ترد را برابر این فایل قرار می‌دهیم و دیگر در تابع `load` فایل را نمی‌بندیم.
در تابع `thread_exit` در صورت وجود `executable_file` برای ترد، این فایل بسته می‌شود.



مدیریت ورودی‌های توابع فراخوانی سیستمی
================
ابتدا در صورت معتبر بودن استک پوینتر، آرگومان‌های قرار داده شده در استک توسط تابع `assign_args` در `syscall.c` بررسی و در صورت معتبر بودن به حافظه‌ی کرنل کپی می‌شوند.

هر کدام از آرگومان‌ها نیز در صورتی که پوینتر باشند و بخواهیم از آن‌ها بخوانیم، پیش از این که به توابع مربوط به خود بروند، در  پرونده‌ی  توسط تابع  `get_kernel_va_for_user_pointer` در همین فایل سنجیده می‌شوند که پوینتر صحیحی باشند و پس از کپی شدن این آدرس از حافظه‌ی کاربر به حافظه‌ی کرنل، آدرس صحیح اختصاص یافته به این پوینتر در حافظه‌ی کرنل برگردانده می‌شود.

برای نوشتن در حافظه‌ی کاربر نیز با استفاده از تابع `validate_user_pointer` بررسی می‌شود بافر داده کاملا در فضای کاربر باشد.

میزان مشارکت
================
پاس دادن آرگومان‌ها: امیرمحمد قاسمی

فراخوانی‌های سیستمی برای کنترل پردازه‌ها: امیر محمد قاسمی و صبا هاشمی

فراخوانی‌های سیستمی برای کنترل پرونده‌ها: مهرانه نجفی و یاشار ظروفچی

مرتب‌سازی کد و دیباگ و بررسی تست‌های کلی filesys: صبا هاشمی

مستند سازی: امیرمحمد قاسمی، یاشار ظروفچی و مهرانه نجفی
