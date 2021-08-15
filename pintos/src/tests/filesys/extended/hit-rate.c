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
