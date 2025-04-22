#include <rthw.h>
#include <rtthread.h>

#define __section_t(S)		__attribute__((__section__(#S)))
#define __versionid		__section_t(.version_id_table)

struct version_id {
	char verid[64];
	int reserved;
} __attribute__((packed, aligned(0x100)));

#include "version_id_gen.cc"

int rt_hw_version_id_init(void)
{
	spacemit_verid.reserved = 0;

	return 0;
}

INIT_BOARD_EXPORT(rt_hw_version_id_init);

