#include "syscall.h"
#include "filesys/filesys.h"
#include "devices/block.h"
#include "tests/lib.h"
#include "tests/main.h"

void test_main(void)
{
    int init_size = 64 * 1024;
    create("samp_64k.txt", init_size);
    int fd = open("samp_64k.txt");
    int bytes_written = 0;
    int bytes_read = 0;
    char *buf[BLOCK_SECTOR_SIZE];
    int expected_wr_cnt = 0;
    int expected_read_cnt = 0;
    memset(buf, 'a', 1);
    // memset(buf, 'a', sizeof buf);
    for (int i = 0; (bytes_written = write(fd, buf, 1)) > 0 && expected_wr_cnt < init_size; i++)
    {
        expected_wr_cnt++;
    }
    seek(fd, 0);
    int wr_cnt = blk_wr_cnt();
    for (int i = 0; (bytes_read = read(fd, buf, 1)) > 0; i++)
    {
        expected_read_cnt++;
    }
    close(fd);
    remove("samp_64k.txt");
    int dif = (blk_wr_cnt() - wr_cnt) ;
    CHECK(dif > 2 * init_size / 1024 && dif < 3 * init_size / 1024, "block_write operation invokation count matches the exception");
}