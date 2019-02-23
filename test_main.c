/*
 * test_main.c
 *
 *  Created on: Feb 26, 2016
 *      Author: roger
 */

#include "elfcore.h"
#include <string.h>

void test_elfcore(void);


uint32_t core_buf[32 * 1024 / 4];
Frame core_frame;


void test_elfcore()
{
    memset(core_buf, 0xde, sizeof(core_buf));
    CreateElfCore("core", 0x1fffc000, (uint8_t *)core_buf, 32*1024, &core_frame);
}

int main(int argc, char *argv[])
{
    test_elfcore();
}
