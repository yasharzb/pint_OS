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

تست دوم به نام `full-bw` پیاده‌سازی شده است. در این تست پس از باز کردن تعداد `read_cnt`ها را بدست می‌آوریم سپس فراخوانی سیستمی `write` را صدا می‌زنیم در نهایت مجدد `blk_cread_cnt` را صدا می‌زنیم و تفاضل آن با پیش از عملیات نوشتهن را بدست می‌آوریم. مطابق انتظار این تفاضل باید صفر باشد که یعنی هیچ رجوعی به تابع `block_read` نبوده است. برای سنجش تعداد نوشتن‌ها هم `SYS_BLK_WR_CNT` به‌عنوان فراخوانی سیستم پیاده‌سازی شده است که تابع `blk_wr_cnt` را صدا می‌زند. خروجی این تابع نیز باید معادل سایز فایل ضربدر دو (بعلت نیم‌کیلوبایتی بودن قطعات دیسک) باشد.‌

پرونده‌های توسعه‌پذیر
============================



پوشه
================




</div>
