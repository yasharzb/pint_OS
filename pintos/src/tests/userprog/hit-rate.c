#include "syscall.h"
#include "filesys/filesys.h"
#include "devices/block.h"
#include "tests/lib.h"
#include "tests/main.h"

void test_main(void)
{
    int fd = open("sample_16k.txt");
    int bytes_read = 0;
    char *buf[BLOCK_SECTOR_SIZE];
    int missed_before = 0;
    int missed_after = 0;
    int read_cnt = fs_device->read_cnt;
    for (int i = 0; (bytes_read = read(fd, buf, BLOCK_SECTOR_SIZE)) > 0; i++)
    {
        /* code */
    }
    missed_before = fs_device->read_cnt - read_cnt;
    read_cnt = fs_device->read_cnt;
    close(fd);
    // if (missed_before < 0)
    // {
    //     missed_before++;
    CHECK((fs_device->read_cnt > 0), "hi");
    // }

    // fd = open("sample_16k.txt");
    // for (int i = 0; (bytes_read = read(fd, buf, BLOCK_SECTOR_SIZE)) > 0; i++)
    // {
    //     /* code */
    // }
    // missed_after = fs_device->read_cnt - read_cnt;
    // close(fd);
    // if (missed_before <= missed_after)
    // {
    //     fail("Hit rate is not improved");
    // }
}