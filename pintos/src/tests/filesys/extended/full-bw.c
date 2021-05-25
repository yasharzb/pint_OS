#include "syscall.h"
#include "filesys/filesys.h"
#include "devices/block.h"
#include "tests/lib.h"
#include "tests/main.h"

void test_main(void)
{
    int fd = open("sample_99k.txt");
    int read_cnt = blk_read_cnt();
    int bytes_written = 0;
    char *buf[BLOCK_SECTOR_SIZE];
    memset(buf, 'a', BLOCK_SECTOR_SIZE * 1);
    for (int i = 0; (bytes_written = write(fd, buf, BLOCK_SECTOR_SIZE)) > 0; i++)
        ;
    close(fd);
    CHECK((blk_read_cnt() - read_cnt) == 0, "block_read operation didn't happen");
}