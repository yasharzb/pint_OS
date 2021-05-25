#include "syscall.h"
#include "filesys/filesys.h"
#include "devices/block.h"
#include "tests/lib.h"
#include "tests/main.h"

void test_main(void)
{
    int fd = open("sample_30k.txt");
    int read_cnt = blk_read_cnt();
    int bytes_read = 0;
    char *buf[BLOCK_SECTOR_SIZE];
    for (int i = 0; (bytes_read = read(fd, buf, BLOCK_SECTOR_SIZE)) > 0; i++)
        ;
    int missed_before = blk_read_cnt() - read_cnt;
    read_cnt = blk_read_cnt();
    seek(fd, 0);
    for (int i = 0; (bytes_read = read(fd, buf, BLOCK_SECTOR_SIZE)) > 0; i++)
        ;
    int missed_after = blk_read_cnt() - read_cnt;
    close(fd);
    CHECK(missed_after < missed_before, "Hitrate is improved");
}
