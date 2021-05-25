<div dir="rtl" align="justify">

# تمرین گروهی ۳ - گزارش نهایی

گروه
-----

یاشار ظروفچی <yasharzb@chmail.ir>

صبا هاشمی <sba.hashemii@gmail.com> 

امیرمحمد قاسمی <ghasemiamirmohammad@yahoo.com> 

مهرانه نجفی <najafi.mehraneh@gmail.com> 



حافظه‌ی نهان بافر
================

پیاده‌سازی تقریبا مشابه آن چه است که در مستند طراحی ذکر شد. البته به‌خاطر بخش تست‌نویسی امضای ساختارهای `inode` و `block` به پرونده‌ی سرآیندشان منتقل شد.

#### پیاده‌سازی خواندن و نوشتن از طریق حافظه‌ی نهان

برای بخش خواندن، تغییرات در تابع `inode_read_at` صورت گرفته است. به این‌صورت که ابتدا با تابع `get_cache` تعریف شده در ساختار `cache`، حافظه‌ی نهان مربوط به آن سکتور گرفته می‌شود و سپس عملیات زیر انجام می‌شود:

<div dir="ltr">

```c
if (ca != NULL)
    {
      memcpy(buffer + bytes_read, ca->data + sector_ofs, chunk_size);
    }
    else
    {
      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
      {
        block_read(fs_device, sector_idx, buffer + bytes_read);
        set_cache(sector_idx, buffer + bytes_read);
      }
      else
      {
        if (bounce == NULL)
        {
          bounce = malloc(BLOCK_SECTOR_SIZE);
          if (bounce == NULL)
            break;
        }
        block_read(fs_device, sector_idx, bounce);
        set_cache(sector_idx, bounce);
        memcpy(buffer + bytes_read, bounce + sector_ofs, chunk_size);
      }
```

</div>

عملیات `memcpy` که کاملا مشخص است؛ در نسخه‌ی پیش از حافظه‌ی نهان نیز این وجود داشته است. تنها تفاوت ساختن حافظه‌ی نهان است.
به طریقی مشابه برای نوشتن نیز چنین چیزی تعبیه‌شده است:

<div dir="ltr">

```c
if (ca != NULL)
    {
      update_cache(ca, buffer + bytes_written, sector_ofs, chunk_size);
    }
    else
    {
      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
      {
        block_write(fs_device, sector_idx, buffer + bytes_written);
        set_cache(sector_idx, buffer + bytes_written);
      }
      else
      {
        if (bounce == NULL)
        {
          bounce = malloc(BLOCK_SECTOR_SIZE);
          if (bounce == NULL)
            break;
        }

        if (sector_ofs > 0 || chunk_size < sector_left)
          block_read(fs_device, sector_idx, bounce);
        else
          memset(bounce, 0, BLOCK_SECTOR_SIZE);
        memcpy(bounce + sector_ofs, buffer + bytes_written, chunk_size);
        block_write(fs_device, sector_idx, bounce);
        set_cache(sector_idx, bounce);
      }
```

</div>

همانطور که مشخص است در اینجا درصورتی که بخواهیم ضرایب صحیحی از بلوک‌ها را بخوانیم، تنها تابع `block_write` استفاده می‌شود (همانطور که در تست سوم گفته‌شده است)، اما اگر آفست باقیمانده‌ای بر مقدار `BLOCK_SECTOR_SIZE` داشته باشد ابتدا مقدار فعلی آن خوانده می‌شود و سپس جمعا نوشته می‌شود.

پیاده‌سازی توابع `update-cache`، `get-cache` و `set-cache` بسیار ساده و معقول است و درصورت نیاز می‌توانید به پیاده‌سازی آن‌ها در `cache.c` رجوع کنید.
الگوریتم ساعت نیز به صورت زیر پیاده‌سازی شده است:

<div dir="ltr">

```c
void apply_clock_algorithm()
{
        cache *ca = NULL;

        if (cache_pointer == NULL || cache_pointer == list_end(&cached_blocks))
                cache_pointer = list_begin(&cached_blocks);

        while (1)
        {
                ca = list_entry(cache_pointer, cache, elem);
                if (ca->is_dirty)
                {
                        write_back(ca);
                }
                if (!ca->is_used)
                {
                        cache_pointer = list_next(cache_pointer);
                        remove_cache(ca);
                        cache_size--;
                        return;
                }
                ca->is_used = 0;
                cache_pointer = list_next(cache_pointer);
                if (cache_pointer == list_end(&cached_blocks))
                        cache_pointer = list_begin(&cached_blocks);
        }
}
```

</div>

همانطور که می‌بینید مکانیزم `write-back` به صورت هرازگاهی در اینجا انجام می‌شود. هر گاه به یک بلوک کثیف رسیدیم عمل بازنویسی در بلوک انجام می‌شود.

#### پیاده‌سازی تست‌های اول و سوم

تست اول به نام `hit-rate` ساخته‌شده است. روال کلی این است که پس از باز کردن فایل تعداد `read_cnt`های انجام شده روی `fs_device` را بررسی می‌کنیم و یک بار عملیات خواندن را انجام می‌دهیم. سپس مجدد `read_cnt` را گرفته و تفاضل را حساب می‌کنیم. اکنون ی نوبت دیگر عملیات خواندن را انجام داده و مجدد تفاضل را بررسی می‌کنیم. درصورتی که در سری دوم تفاضل کم‌تر بود یعنی دفعات کمتری سراغ تابع `block_read` رفته ایم پس نتیجتا نرخ برخورد بالاتری داشته‌ایم. ازآنجایی که تست‌ها در فضای کاربر هستند، یک فراخوانی سیستمی `SYS_BLK_READ_CNT` اضافه شده‌است که از طریق تابع `blk_read_cnt` عملیات خود را انجام می‌دهد. جزئیات مربوط به آن‌ها در `syscall.c`های پوشه‌های `lib` و `userprog` وجود دارد.

خروجی تست اول:

<div dir="ltr">

```
Copying tests/filesys/extended/hit-rate to scratch partition...
Copying tests/filesys/extended/tar to scratch partition...
Copying ../../tests/sample_30k.txt to scratch partition...
qemu-system-i386 -device isa-debug-exit -hda /tmp/Guk72JAgnI.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1^M
Loading............^M
Kernel command line: -q -f extract run hit-rate
Pintos booting with 3,968 kB RAM...
367 pages available in kernel pool.
367 pages available in user pool.
Calibrating timer...  574,259,200 loops/s.
hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
hda1: 192 sectors (96 kB), Pintos OS kernel (20)
hda2: 301 sectors (150 kB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'hit-rate' into the file system...
Putting 'tar' into the file system...
Putting 'sample_30k.txt' into the file system...
Erasing ustar archive...
Executing 'hit-rate':
(hit-rate) begin
(hit-rate) Hitrate is improved
(hit-rate) end
hit-rate: exit(0)
Execution of 'hit-rate' complete.
Timer: 75 ticks
Thread: 45 idle ticks, 29 kernel ticks, 1 user ticks
hdb1 (filesys): 108 reads, 608 writes
hda2 (scratch): 300 reads, 2 writes
Console: 1068 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
```

</div>

تست دوم به نام `full-bw` پیاده‌سازی شده است. در این تست پس از باز کردن تعداد `read_cnt`ها را بدست می‌آوریم سپس فراخوانی سیستمی `write` را صدا می‌زنیم در نهایت مجدد `blk_cread_cnt` را صدا می‌زنیم و تفاضل آن با پیش از عملیات نوشتهن را بدست می‌آوریم. مطابق انتظار این تفاضل باید صفر باشد که یعنی هیچ رجوعی به تابع `block_read` نبوده است. برای سنجش تعداد نوشتن‌ها هم `SYS_BLK_WR_CNT` به‌عنوان فراخوانی سیستم پیاده‌سازی شده است که تابع `blk_wr_cnt` را صدا می‌زند. خروجی این تابع نیز باید معادل سایز فایل ضربدر دو (بعلت نیم‌کیلوبایتی بودن قطعات دیسک) باشد.‌

خروجی تست سوم:

<div dir="ltr">

```
Copying tests/filesys/extended/full-bw to scratch partition...
Copying tests/filesys/extended/tar to scratch partition...
Copying ../../tests/sample_1hk.txt to scratch partition...
qemu-system-i386 -device isa-debug-exit -hda /tmp/bkkoOQNVu3.dsk -hdb tmp.dsk -m 4 -net none -nographic -monitor null
PiLo hda1^M
Loading............^M
Kernel command line: -q -f extract run full-bw
Pintos booting with 3,968 kB RAM...
367 pages available in kernel pool.
367 pages available in user pool.
Calibrating timer...  569,344,000 loops/s.
hda: 1,008 sectors (504 kB), model "QM00001", serial "QEMU HARDDISK"
hda1: 192 sectors (96 kB), Pintos OS kernel (20)
hda2: 441 sectors (220 kB), Pintos scratch (22)
hdb: 5,040 sectors (2 MB), model "QM00002", serial "QEMU HARDDISK"
hdb1: 4,096 sectors (2 MB), Pintos file system (21)
filesys: using hdb1
scratch: using hda2
Formatting file system...done.
Boot complete.
Extracting ustar archive from scratch device into file system...
Putting 'full-bw' into the file system...
Putting 'tar' into the file system...
Putting 'sample_1hk.txt' into the file system...
Erasing ustar archive...
Executing 'full-bw':
(full-bw) begin
(full-bw) block_read operation didn't happen
(full-bw) block_write operation invokation count matches the exception
(full-bw) end
full-bw: exit(0)
Execution of 'full-bw' complete.
Timer: 78 ticks
Thread: 48 idle ticks, 29 kernel ticks, 1 user ticks
hdb1 (filesys): 48 reads, 1088 writes
hda2 (scratch): 440 reads, 2 writes
Console: 1146 characters output
Keyboard: 0 keys pressed
Exception: 0 page faults
Powering off...
~                      
```

</div>

درمورد تجربه نوشتن تست برای pintos هم باید گفت:
یاشار لذت برد.

به صورت کلی تست‌های pintos از ۳ بخش تشکیل شده‌اند:
* کد c که منطق تست را داراست و می‌توان از ماکروهای `CHECK` و `ASSERT` برای سنجش خروجی استفاده کرد. 
* با استفاده از زبان پرل(؟) یک فایل `ck` نوشته می‌شود که در بدنه‌ی آن خروجی موردانتظار چاپ‌شده‌ی تست قرار دارد.
* یک فایل `test-persistence.ck` که  خود مستند انگلیسی هم چندان توضیح مناسبی نداده بود اما بهرحال حضورش برای تست لازم است.

ایرادهای هسته:
* یکی اینکه اسم فایل ورودی می‌بایست کمترمساوی ۱۴ تا باشد. هنگامی که تست `full-bw` را نوشتم در ابتدا برای فایل ۱۰۰ کیلوبایتی از نام `sample-100k.txt` استفاده کردم که دچار وحشت هسته شد.
* یک ایراد هم موقع نوشتن تابع `update-cache` بود که آفست‌گذاری اشتباه انجام شده بود با کمک تست‌ها مشکلش رفع شد.


پرونده‌های توسعه‌پذیر
============================

پیاده سازی این بخش  تغییراتی را نسبت به طراحی داشته. در طراحی در نظرداشتیم تا هر inode دارای تعدادی سکتور مستقیم و یک بلوک اشاره‌گر یک مرحله‌ای و یک بلوک اشاره‌گر دومرحله‌ای داشته باشد. ساختار inode در زمان طراحی چنین بوده‌است:


<div dir='ltr'>

```c
struct inode_disk
{
    off_t length;
    unsigned magic;
    int type;
    block_sector_t direct_blocks[16];
    block_sector_t indirect_block;
    block_sector_t double_indirect_block;
    uint32_t unused[100];
};

struct pointer_block 
{
    block_sector_t blocks[BLOCK_SECTOR_SIZE / 4];
};

struct inode 
{
    ...
    struct lock *access_lock; // lock for read/write synchronization
}
  ```

</div>


همانطور که در بالاتر گفتم، inode دیگر سکتورهای مستقیم و بلوک اشاره‌گر یک مرحله‌ای ندارد. وجود این اشاره‌گر‌‌ها باعث تسریع دسترسی و عملیات های سیستم عامل برای تغییرات در فایل های کوچک میشود. در واقع برای تغییرات اولیه در یک فایل کوچک، عملا با سکتور‌های مستقیم سر و کار داریم و بار کاری کمتری در سمت سیستم عامل و دیسک سخت قرار دارد و بنابراین سرعت انجام عملیات بیشتر است.  در پیاده سازی برای این بخش لازم بود که ۴ تابع inode_read_at، inode_write_at، inode_create و inode_close  تغییر میکردند تا از پرونده‌های توسعه پذیر پشتیبانی کنند. در هر یکی از این توابع  عملا نیازاست تا به صورت بهینه و بدون نشتی حافظه عملیات دسترسی به سکتور ها و بلوک های اشاره‌گر را انجام دهیم. با فرض داشتن تنها یک بلوک اشاره‌گر دومرحله‌ای، همواره یک روش برای دسترسی به یک بایت خاص از فایل خواهیم داشت. یعنی از بلوک اشاره‌گر دو مرحله‌ای به بلوک اشاره گر یک مرحله‌ای برسیم و بعد به سکتور مورد نظرو بایت مورد نظر. اما با فرض داشتن بلوک های یک مرحله ای  و مستقیم دسترسی به بایت های مختلف بسته به مکان آنها رویه ای متفاوت دارد. برای بایت های با شماره کوچک باید سکتور های مستقیم را بررسی کنیم و برای کمی بزرگتر بلوک غیر مسقتیم و ... . 
در نهایت ما تنها برای سهولت در کد زدن و کمتر شدن خطا ها و باگ ‌ها از پیاده سازی سکتور های مستقیم و بلوک اشارهگر یک مرحله‌ای غیر مستقیم صرف نظر کردیم. توجه شود که همچنان از حداکثر اندازه پرونده ۲۳^۲ بایت پشتیانی میشود. 

پوشه
================

برای پشتیبانی از مسیرهای مختلف از تابعی به نام `get_name_and_dir_from_path` استفاده شده است. در این تابع مسیر دلخواهی داده می‌شود و توسط تابع `get_path_next_token` مسیر به توکن‌های مختلفی (قسمت‌های مختلف یک مسیر که شامل دایرکتوری‌های مختلف هستند) تبدیل می شود و به این ترتیب در مسیر داده شده جلو می‌رویم. دو چیز مهم از این تابع مقدار دهی می‌شوند: اولا استراکت دایکتوری پدر مقصد نهایی مشخص می‌شود و دوما `inode ` مسیر نهایی برگردانده می‌شود (چه دایرکتوری باشد چه یک فایل). از این تابع در تابع‌های مختلف که ورودی‌شان یک مسیر است (مثلا filesys_remove، filesys_open) استفاده می‌شود تا پوشه‌ها را هم پشتیبانی کنیم. در صورتی که مسیر به ریشه اشاره کند از تابع `is_root_path` استفاده می‌کنیم.


پشتیبانی از `mkdir`، `chdir`، `isdir` و `readdir`
-------------

توابع زیر در پرونده‌ی file-descriptor.c اضافه شده اند:

<div dir='ltr'>

```c
  bool fd_readdir(int fd, void* buffer);
  bool fd_isdir(int fd);
  bool ch_dir(const char* path);
  bool mk_dir(const char* path);
  bool read_dir(file_descriptor* fd, void* buffer);

```
</div>

تابع fd_readdir پس از پیدا کردن دایرکتوری مناسب و اطمینان از صحت مسیر، عملیات readdir را انجام می‌دهد.

برای کار کردن isdir ساختار file descriptor ها به صورت زیر تغییر کرده است:

<div dir = 'ltr'>

```c
  typedef struct file_descriptor
  {
    struct file *file;
    struct dir *dir;
    char *file_name;
    int fd;
    struct list_elem fd_elem;
  } file_descriptor;

```

</div>
 یعنی یک دایرکتوری هم file descriptor محسوب می‌شود. پس از پیدا کردن استراکتی که عدد fd ‌آن برابر با ورودی است، چک می‌کنیم اگر قسمت dir آن null نبود، این file descriptor یک دایرکتوری است.

برای chdir دایرکتوری جدید را به صورت یک inode دریافت می‌کنیم سپس دایرکتوری مربوط به این inode‌را باز می‌کنیم و سپس working_directory ترد فعلی را برابر این دایرکتوری قرار می‌دهیم و دایرکتوری قبلی را می‌بندیم.

برای mkdir هم صرفا تابع sysfile_create() را صدا می‌زنیم.

</div>
