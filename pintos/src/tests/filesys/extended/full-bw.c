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