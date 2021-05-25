#include "syscall.h"
#include "filesys/filesys.h"
#include "devices/block.h"
#include "tests/lib.h"
#include "tests/main.h"

void test_main(void)
{
    int fd = open("sample_64k.txt");
    int bytes_written = 0;
    int bytes_read = 0;
    char *buf[BLOCK_SECTOR_SIZE];
    int expected_wr_cnt = 0;
    int expected_read_cnt = 0;
    memset(buf, 'a', 1);
    // memset(buf, 'a', sizeof buf);
    for (int i = 0; (bytes_written = write(fd, buf, 1)) > 0 && expected_wr_cnt < 64*1024; i++)
    {
        expected_wr_cnt++;
    }
    seek(fd, 0);
    int wr_cnt = blk_wr_cnt();
    for (int i = 0; (bytes_read = read(fd, buf, BLOCK_SECTOR_SIZE)) > 0; i++)
    {
        expected_read_cnt++;
    }
    close(fd);
    msg("%d %d", expected_wr_cnt, blk_wr_cnt() - wr_cnt);
    CHECK((blk_wr_cnt() - wr_cnt) == expected_wr_cnt, "block_write operation invokation count matches the exception");
}
