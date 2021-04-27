<div dir="rtl" align="justify">

# تمرین گروهی ۲ - گزارش نهایی

گروه
-----

یاشار ظروفچی <yasharzb@chmail.ir>

صبا هاشمی <sba.hashemii@gmail.com> 

امیرمحمد قاسمی <ghasemiamirmohammad@yahoo.com> 

مهرانه نجفی <najafi.mehraneh@gmail.com> 



زنگ ساعت دار بهینه
================

همانطور که در مستند طراحی عنوان شد، پیاده‌سازی به این صورت است که بدون علاف شدن برنامه سر یک ریسه‌ی خاص یا به عبارتی همان `busy_waiting` به صورت منطقی بررسی کنیم ببینیم که از زمان خواب هر ریسه گذشته است یا خیر. در صورت اینکه گذشته باشد وی را بیدار کنیم.
به این منظور دو فیلد به ساختار `thread` اضافه شدند. یکی `target_ticks` که تیک نهایی ریسه (یعنی زمانی که باید بیدار شود نسبت به `timer`) و دیگری `alarm_elem` که نماینده‌ی ریسه در لیست ریسه‌های درحال انتظار است.
ابتدا به تغییرات تابع `timer_sleep` اشاره می‌کنیم که تکه‌ی حلقه‌ی `while` آن حذف شده و تابع `thread_push_block` را در آن صدا می‌زنیم. همچنین به انتهای تابع `timer_interrupt`  نیز تابع `thread_pop_block` اضافه شده است که ریسه‌های آماده را بیدار می‌کند.
امضای توابع یاد شده به صورت زیر است

<div dir="ltr">

```c
void thread_push_block(struct thread *t);
```

```c
void thread_pop_unblock();
```
</div>

در تابع اول به صورت مرتب شده یک ریسه وارد لیست انتظار می‌شود. جهت تعریف مفهوم ترتیب، تابع زیر در `thread.c` پیاده‌سازی شده است

<div dir="ltr">

```c
bool cmp_target_ticks(const struct list_elem *a, const struct list_elem *b, void *aux);
```
</div>

خروجی این تابع `bool` است که البته در ساختار `list` یک تعریف نوع انجام شده به نام `list_less_func` که از همین نوع `bool` است.
پس از عمل `list_insert_ordered` ریسه را با تابع `thread_block` تغییر وضعیت می‌دهیم.
در تابع دوم، روی ریسه‌هایی موجود در لیست پیمایش می‌کنیم. هر کدام اگر وضعیت `THREAD_BLOCKED` داشتند و از زمان مقررشان گذشته بود ابتدا `thread_unblock` می‌کنیم و سپس از لیست حذف می‌شوند.

زمان‌بند اولویت‌دار
============================
برای پیاده‌سازی این قسمت فیلدهای زیر که در طراحی نیز ذکر شده بودند به استراکت thread اضافه شدند. کاربرد هر یک نیز نوشته شده است.

 فیلد `priority_lock` که در داک طراحی آورده شده بود حذف شده است. علت آن این است که در جاهایی که حدس زده بودیم نیاز به استفاده از این قفل می‌شود interrupt ها غیر فعال بودند و دیگر نیازی به استفاده از قفل نبود.
<div dir="ltr">

```c
int effective_priority;         /* Thread's effective priority */
struct list holding_locks_list; /* List of locks that thread is holding */
struct lock *waiting_lock;      /* Lock that thread is waiting to acquire */
   
```
</div>

برای این که مطمئن شویم در زمان‌بندی و بیدار کردن waiter های یک سمافور همواره ترد با بالاترین اولویت را انتخاب می‌کنیم تابع‌های زیر تعریف شد:

<div dir="ltr">

```c
bool thread_priority_less_function (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);

struct thread *get_and_remove_next_thread (struct list *list);
```
</div>

تابع `thread_priority_less_function` از نوع `list_less_func` است و دو ترد را بر حسب اولویت موثر مقایسه می‌کند.

در تابع `get_and_remove_next_thread` با استفاده از تابع بالا از لیست داده شده ترد با بالاترین اولویت را خارج کرده و return می‌کنیم.
این تابع در `sema_up` و `next_thread_to_run` استفاده می‌شود.

waiter های یک کاندیشن به صورت لیستی از سمافورها ذخیره می‌شدند برای همین نمی‌توانستیم از دو تابع بالا برای انتخاب ترد بعدی استفاده کنیم. برای همین دو تابع `cond_priority_less_function` و `get_and_remove_next_sema_for_cond` تعریف شد که کاربردی مشابه دو تابعی که پیش‌تر ذکر شد دارند و برای انتخاب ترد با بالاترین اولویت در صف یک کاندیشن از آن‌ها استفاده می‌شود.

برای پیاده‌سازی اهدای اولویت همانند چیزی که در داک طراحی گفته شده بود از دو تابع زیر استفاده شده است:
<div dir="ltr">

```c
void compare_priority_and_update(struct thread *t, int priority);
void calculate_priority_and_yield(struct thread *t); 

```
</div>

با توجه به این که تغییر خاصی در این توابع و تابع‌های ` lock_acquire` و `lock_release` نسبت به چیزی که در طراحی گفته شد نداشتیم از ذکر مجدد آن خودداری می‌شود.


یک نکته‌ی دیگر که در طراحی ذکر نشده بود این است که پس از `sema_up` نیز در صورتی که ترد بیشترین اولویت را نداشته باشد `thread_yield` را صدا می‌زنیم تا ترد با بالاترین اولویت اجرا شود. در طراحی فقط به انجام این کار برای قفل‌ها اشاره کرده بودیم؛ ولی لازم بود که برای کاندیشن‌ها و سمافورها نیز این کار را انجام دهیم. (در کاندیشن‌ نیز از sema_up استفاده می‌شود.)
هم چنین در انتهای تابع `thread_create` نیز در صورت لزوم `thread_yield` صدا زده می‌شود.

آزمایشگاه زمان‌بندی
================

پاسخ‌ها در فایل PDF از پوشه‌ی report وجود دارند.


میزان مشارکت
================

</div>
