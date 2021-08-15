<div dir="rtl">

حافظه‌ی نهان بافر
================

#### پیاده‌سازی خواندن و نوشتن از طریق حافظه‌ی نهان

برای بخش خواندن، تغییرات در تابع `inode_read_at` صورت گرفته است. به این‌صورت که ابتدا با تابع `get_cache` تعریف شده در ساختار `cache`، حافظه‌ی نهان مربوط به آن سکتور گرفته می‌شود و سپس عملیات زیر انجام می‌شود:

<div dir="ltr">

```c
    if (ca != NULL)
    {
      memcpy(buffer + bytes_read, ca->data + sector_offset, chunk_size);
    }
    else
    {
      if (sector_offset == 0 && chunk_size == BLOCK_SECTOR_SIZE)
      {
        block_read(fs_device, indirect_block_buffer->blocks[sector_ind_in_indirect_block], buffer + bytes_read, true);
        set_cache(sector, buffer + bytes_read);
      }
      else
      {
        if (sector_buffer == NULL)
          sector_buffer = malloc(BLOCK_SECTOR_SIZE);
        if (sector_buffer == NULL)
          break;
        block_read(fs_device, sector, sector_buffer, true);
        set_cache(sector, sector_buffer);
        memcpy(buffer + bytes_read, sector_buffer + sector_offset, chunk_size);
      }
    }
```

</div>

عملیات `memcpy` که کاملا مشخص است؛ در نسخه‌ی پیش از حافظه‌ی نهان نیز این وجود داشته است. تنها تفاوت ساختن حافظه‌ی نهان است.
به طریقی مشابه برای نوشتن نیز چنین چیزی تعبیه‌شده است:

<div dir="ltr">

```c
    if (ca != NULL)
    {
      update_cache(ca, buffer + bytes_written, sector_offset, chunk_size);
    }
    else
    {
      if (sector_offset == 0 && chunk_size == BLOCK_SECTOR_SIZE)
      {
        block_write(fs_device, indirect_block_buffer->blocks[sector_ind_in_indirect_block], buffer + bytes_written, false);
        set_cache(sector, buffer + bytes_written);
      }
      else
      {
        if (sector_buffer == NULL)
          sector_buffer = malloc(BLOCK_SECTOR_SIZE);
        if (sector_buffer == NULL)
          break;
        block_read(fs_device, sector, sector_buffer, true);
        memcpy(sector_buffer + sector_offset, buffer + bytes_written, chunk_size);
        block_write(fs_device, sector, sector_buffer, false);
        set_cache(sector, sector_buffer);
      }
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

جهت بررسی فرآیند `read_ahead` تابع `inode_read_ahead` به صورت زیر نوشته شده است:

<div dir="ltr">

```c
void inode_read_ahead(void *aux)
{
  block_sector_t index = *(int *)aux;
  struct cache *ce;
  uint8_t buffer[BLOCK_SECTOR_SIZE];

  for (int i = 1; i <= READ_AHEAD_SIZE; i++)
  {
    ce = get_cache(index + i);
    if (ce == NULL)
    {
      block_read(fs_device, index + i, buffer, true);
      set_cache(index + i, buffer);
    }
  }
}
```

</div>

الگوریتم این بخش تقریبا مشخص است. هر گاه تصمیم به خواندن بلوکی گرفته شد، ۳ بلوک جلوتر (تعریف شده به عنوان ماکروی `READ_AHEAD_SIZE`) نیز خوانده می‌شود.

#### پیاده‌سازی تست‌های اول و دوم

تست اول به نام `hit-rate` ساخته‌شده است. روال کلی این است که پس از باز کردن فایل تعداد `read_cnt`های انجام شده روی `fs_device` را بررسی می‌کنیم و یک بار عملیات خواندن را انجام می‌دهیم. سپس مجدد `read_cnt` را گرفته و تفاضل را حساب می‌کنیم. اکنون ی نوبت دیگر عملیات خواندن را انجام داده و مجدد تفاضل را بررسی می‌کنیم. درصورتی که در سری دوم تفاضل کم‌تر بود یعنی دفعات کمتری سراغ تابع `block_read` رفته ایم پس نتیجتا نرخ برخورد بالاتری داشته‌ایم. ازآنجایی که تست‌ها در فضای کاربر هستند، یک فراخوانی سیستمی `SYS_BLK_READ_CNT` اضافه شده‌است که از طریق تابع `blk_read_cnt` عملیات خود را انجام می‌دهد. جزئیات مربوط به آن‌ها در `syscall.c`های پوشه‌های `lib` و `userprog` وجود دارد. همچنین برای عدم تغییر باقی استفاده‌ها از `read`، یک فراخوانی سیستمی `SYS_READ_WITH_AHEAD` تعریف شده است که علاوه بر خواندن معمولی سراغ واکشی بلوک‌های جلوتر نیز می‌رود. استفاده از `SYS_TICK` هم برای بدست آوردن گذر تایمر است زیرا در لایه‌ی کاربر توابع `timer` قابل استفاده نیستند.

کد تست اول:

<div dir="ltr">

```c
#include "syscall.h"
#include "filesys/filesys.h"
#include "devices/block.h"
#include "tests/lib.h"
#include "tests/main.h"

int64_t measure_time_for_read(char *f_name, int coef, bool with_ahead);
void read_loop(bool with_ahead, int fd, bool seq);

void test_main(void)
{
    int coef = 60;
    int64_t t1 = measure_time_for_read("smp_30k.txt", coef, false);
    int64_t t2 = measure_time_for_read("smp2_30k.txt", coef, true);
    CHECK(t2 < t1, "Timing is improved");
}

int64_t measure_time_for_read(char *f_name, int coef, bool with_ahead)
{
    int fd = open(f_name);
    int read_cnt = blk_read_cnt();
    int64_t ticks = timer_ticks();
    read_loop(with_ahead, fd, false);
    ticks = timer_ticks() - ticks;
    int missed_before = blk_read_cnt() - read_cnt;
    read_cnt = blk_read_cnt();
    seek(fd, 0);
    read_loop(with_ahead, fd, true);
    int missed_after = blk_read_cnt() - read_cnt;
    close(fd);
    CHECK(missed_before - missed_after == coef, "Hitrate is improved");
    return ticks;
}

void read_loop(bool with_ahead, int fd, bool seq)
{
    int bytes_read = 0;
    char *buf[BLOCK_SECTOR_SIZE];
    if (with_ahead)
    {
        if (seq)
        {
            while ((bytes_read = read(fd, buf, BLOCK_SECTOR_SIZE)))
                ;
        }
        else
        {
            while ((bytes_read = read_with_ahead(fd, buf, BLOCK_SECTOR_SIZE)))
            {
                seek(fd, tell(fd) + READ_AHEAD_SIZE * BLOCK_SECTOR_SIZE);
            }
        }
    }
    else
    {
        for (int i = 0; (bytes_read = read(fd, buf, BLOCK_SECTOR_SIZE)) > 0; i++)
            ;
    }
}

```
</div>

خروجی تست اول:

<div dir="ltr">

```
Executing 'hit-rate':
(hit-rate) begin
(hit-rate) Hitrate is improved
(hit-rate) Hitrate is improved
(hit-rate) Timing is improved
(hit-rate) end
hit-rate: exit(0)
Execution of 'hit-rate' complete.
```

</div>

تست دوم به نام `full-bw` پیاده‌سازی شده است. در این تست ابتدا یک فایل ایجاد می‌کنیم سپس آن را باز کرده و بایت به بایت می‌نویسیم تا تمامی کش‌ها کثیف شوند. سپس بایت به بایت شروع به خواندن می‌کنیم. چون تابع `set_cache` مدام صدا زده می‌شود و تمامی کش‌ها کثیف هستند پس مرتبا `write_back` خواهیم داشت.‌

کد تست دوم:

<div dir="ltr">

```c

#include "syscall.h"
#include "filesys/filesys.h"
#include "devices/block.h"
#include "tests/lib.h"
#include "tests/main.h"

void test_main(void)
{
    int coef = 128;
    int init_size = coef * BLOCK_SECTOR_SIZE;
    create("samp_64k.txt", init_size);
    int fd = open("samp_64k.txt");
    int bytes_written = 0;
    int bytes_read = 0;
    char *buf[BLOCK_SECTOR_SIZE];
    int expected_wr_cnt = 0;
    int expected_read_cnt = 0;
    memset(buf, 'a', BLOCK_SECTOR_SIZE);
    // memset(buf, 'a', sizeof buf);
    int init_w = blk_wr_cnt();
    int init_r = blk_read_cnt();
    for (int i = 0; expected_wr_cnt < init_size && (bytes_written = write(fd, buf, BLOCK_SECTOR_SIZE)) > 0; i++)
    {
        expected_wr_cnt += BLOCK_SECTOR_SIZE;
    }
    int difi_w = blk_wr_cnt() - init_w;
    int difi_r = blk_read_cnt() - init_r;
    seek(fd, 0);
    int wr_cnt = blk_wr_cnt();
    int r_cnt = blk_read_cnt();
    for (int i = 0; i < expected_wr_cnt; i++)
    {
        read(fd, buf, 1);
    }
    int dif_w = (blk_wr_cnt() - wr_cnt);
    int dif_r = (blk_read_cnt() - r_cnt);
    close(fd);
    remove("samp_64k.txt");
    CHECK(dif_r == coef, "block_write operation invokation count matches the exception");
}
```
</div>

خروجی تست دوم:

<div dir="ltr">

```
Executing 'full-bw':
(full-bw) begin
(full-bw) block_write operation invokation count matches the exception
(full-bw) end
full-bw: exit(0)
Execution of 'full-bw' complete.
```

</div>

</div>