#include "syscall.h"
#include "filesys/filesys.h"
#include "devices/block.h"
#include "tests/lib.h"
#include "tests/main.h"

void test_main(void)
{
    int fd = open("sample_1hk.txt");
    int read_cnt = blk_read_cnt();
    int wr_cnt = blk_wr_cnt();
    int bytes_written = 0;
    char *buf[BLOCK_SECTOR_SIZE];
    int expected_wr_cnt = 0;
    // memset(buf, 'a', BLOCK_SECTOR_SIZE * 1 );
    memset(buf, 'a', sizeof buf);
    for (int i = 0; (bytes_written = write(fd, buf, BLOCK_SECTOR_SIZE)) > 0; i++){
        expected_wr_cnt++;
    }
    close(fd);
    CHECK((blk_read_cnt() - read_cnt) == 0, "block_read operation didn't happen");
    CHECK((blk_wr_cnt() - wr_cnt) == expected_wr_cnt, "block_write operation invokation count matches the exception");
}
