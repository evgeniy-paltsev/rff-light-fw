#ifndef DYNAMIC_ASSERT_H_
#define DYNAMIC_ASSERT_H_

#include <sys/printk.h>
#include "rtc/rtc-ctl.h"

#define dynamic_assert(cond_true) ({											\
	if (!(cond_true)) {												\
		uint32_t time = rtc_get_time();										\
		while (true) {												\
			printk(" ! assert: (%s), %s %d, time %u\n", #cond_true, __func__, __LINE__, time);		\
		}													\
	}														\
})

#endif
