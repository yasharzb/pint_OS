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

برای قسمت دوم، تابع جدید `alternative_setup_stack` را با امضا (signature) زیر به جای تابع `setup_stack` استفاده کردیم.

<div dir="ltr">

```c
bool alternative_setup_stack(int argc, char **argv, void **esp)
```
</div>

 در این تابع مانند تابع `s‍‍‍‍‍etup_stack`، یک صفحه حافظه متناظر با پشته کاربر ایجاد میکنیم و آرگومان ها را در آن قرار میدهیم. همچنین اندازه پشته را با اندازه صفحه حافظه مقایسه میکنیم و اگر صفحه به اندازه کافی فضا برای قرار دادن آرگومان ها نداشت، مقدار false را برمیگردانیم. 
 
 طول پشته کاربر با استفاده از طول آرگومان‌ها و تعدادشان وهمچنین تعداد بایت‌های لازم برای تراز کردن پشته کاربر مشخص میشود. دو مرحله ترازسازی پشته کاربر نیاز داریم. در مرحله اول باید اطمنین حاصل کنیم که تعدادی بایت خالی پس از درج آرگومان‌ها به پشته اضافه کنیم که در مجموع مضربی از ۴ برای رشته‌ها استفاده کرده باشیم. در مرحله دوم باید اطمینان حاصل کنیم در نهایت اشاره‌گر پشته در مضربی از ۱۶ قرار میگیرد. پس مقداری را بدست می‌آوریم که با اضافه شدن آن مقدار به بالای پشته، درنهایت اشاره‌گر پشته در مضربی از ۱۶ قرار بگیرد.

در نهایت لازم است به این نکته اشاره کنیم که هر صفحه‌ای از حافظه که با تابع `palloc_get_page` گرفته شده در صورت بروز خطا (مثلا جا نشدن آرگومان ها در استک) و یا به اتمام رسیدن کار (مثلا صفحه های استفاده شده برای تجزیر آرگومان ها) آزاد شده.


فراخوانی‌های سیستمی
================

برای فراخوانی های سیستمی بر روی پردازه‌ها،‌اعم از exit, wait, exec, halt کار های زیر انجام شده‌است.
مطابق مستند طراحی، اعضای  زیر را به ساختار thread اضافه کردیم.


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
    
    int fd_counter;
    struct list fd_list;
}
```
</div>


این اعضا در تابع `thread_create` مقداره دهی اولیه میشوند. پدر ریسه ای که در حال ایجاد شدن در تابع `thread_create` است  همان ریسه‌ای هست که دارد این تابع را اجرا میکند. پس پدر یک ریسه بدین صورت و با تابع `thread_current`مشخص میشود و ریسه جدید در لیست فرزندان ریسه پدر درج میشوند.

برای پیاده سازی  exec، تابعی با همین نام در `syscall.c` اضافه کرده‌ایم که مسئولیت دارد که آرگومان ورودی به آن را عنوان یک دستور  اجرا کند. به همین منظور این تابع در ابتدا `process_execute` را با آرگومان ورودی‌اش صدا میکند. در این جا میدانیم که خروجی  `process_execute` یک ‍`tid_t` است. پس برای دسترسی به خود ساختار ریسه تابع `get_thread` در `thread.c` اضافه شده که یک ورودی از جنس `tid_t` میگیرد و ریسه‌ای با این آیدی را خروجی میدهد. چون لازم است که ابتدا پرونده اجرایی جدید بارگذاری شود و بعد به برنامه کاربر برگردیم، در این جا سمافور `load_done` فرزند را کاهش میدهدیم.  این سمافور در تابع `start_process` بعد از این که از نتیجه بارگذاری خبردار شدیم، افزایش میباید و بدین ترتیب تابع exec از وضعیت بارگذاری پرونده اجرایی فرزند آگاه میشود و به برنامه کابر نتیجه را اعلام میکند.

برای پیاده سازی wait، تابع `process_wait` در `process.c‍‍` تکمیل شد که فرآیند آن دقیقا مشابه مستند طراحی است(در بخش همگام سازی توضیحت آن وجود دارد ). به طور خلاصه از سمافور ‍`exited` برای اطلاع به ریسه پدر هنگام خروجی ریسه فرزند استفاده میشود. این سمافور در `process_wait‍` کاهش میباید. بدین صورت پدر از مقدار خروجی ریسه فرزند آگاه میشود و این جاست که ریسه پدر سمافور `can_free` را افزایش میدهد و فرزند فرآیند خروج خود را طی میکند. بررسی هایی از این دست که ورودی تابع `process_wait‍‍` معتبر باشد و دوبار بر روی ریسه فرزند توفق نکرده باشیم نیز انجام میشوند. 
همچنین هنگام خروج یک ریسه، سمافور `can_free` برای تمام فرزندان آن ریسه افزایش میباید.

** ورودی‌های این توابع در صورتی که پوینتر باشند، پیش ازینکه به توابع مربوط به خود بروند، در  پرونده‌ی syscall.c توسط تابع get_kernel_va_for_user_pointer سنجیده می‌شوند که پوینتر صحیحی باشند و پس از کپی شدن این آدرس از حافظه‌ی کاربر به حافظه‌ی کرنل، آدرس صحیح اختصاص یافته به این پوینتر در حافظه‌ی کرنل را برمی‌گرداند.


فراخوانی سیستمی روی پرونده‌ها
============================

>دراینجا توضیحات مربوط به `file-descriptor` قرار می‌گیرد.


پرونده‌ای به نام `file-descriptor` در آدرس `pintos/src/filesys` قرار گرفته که عملیات‌های مربوط به `file descriptor` ها در آن نوشته شده است.


<div dir="ltr">

```c
typedef struct file_descriptor
{
   struct file *file;
   char *file_name;
   int fd;
   bool closed;
   bool removed;
   struct list_elem fd_elem;
} file_descriptor;

```
</div>

در این بخش به نحوه‌ی پیاده‌سازی هر کدام از فراخوانی‌های فایلی خواسته شده می‌پردازیم

### `create`

### `remove`

### `open`

### `close`

### `write`

### `read`

### `filesize`

### `seek`

### `tell`

میزان مشارکت
================