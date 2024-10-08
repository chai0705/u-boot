/*
 * Copyright (C) 2015 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <sysreset.h>
#include <dm.h>
#include <errno.h>
#include <regmap.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <dm/root.h>
#include <linux/err.h>

#ifdef CONFIG_ARCH_ROCKCHIP
__weak void reset_misc(void)
{
}
#endif

int sysreset_request(struct udevice *dev, enum sysreset_t type)
{
	struct sysreset_ops *ops = sysreset_get_ops(dev);

	if (!ops->request)
		return -ENOSYS;

	return ops->request(dev, type);
}

int sysreset_walk(enum sysreset_t type)
{
	struct udevice *dev;
	int ret = -ENOSYS;

	while (ret != -EINPROGRESS && type < SYSRESET_COUNT) {
		for (uclass_first_device(UCLASS_SYSRESET, &dev);
		     dev;
		     uclass_next_device(&dev)) {
			ret = sysreset_request(dev, type);
			if (ret == -EINPROGRESS)
				break;
		}
		type++;
	}

	return ret;
}

static void sysreset_walk_reboot_mode(const char *mode)
{
	struct sysreset_ops *ops;
	struct udevice *dev;

	if (!mode)
		return;

	for (uclass_first_device(UCLASS_SYSRESET, &dev);
	     dev;
	     uclass_next_device(&dev)) {
		ops = sysreset_get_ops(dev);
		if (ops && ops->request_by_mode) {
			ops->request_by_mode(dev, mode);
			break;
		}
	}
}

void sysreset_walk_halt(enum sysreset_t type)
{
	int ret;

	ret = sysreset_walk(type);

	/* Wait for the reset to take effect */
	if (ret == -EINPROGRESS)
		mdelay(100);

	/* Still no reset? Give up */
	debug("System reset not supported on this platform\n");
	hang();
}

/**
 * reset_cpu() - calls sysreset_walk(SYSRESET_WARM)
 */
void reset_cpu(ulong addr)
{
	sysreset_walk_halt(SYSRESET_WARM);
}

void reboot(const char *mode)
{
	sysreset_walk_reboot_mode(mode);
	flushc();
	sysreset_walk_halt(SYSRESET_COLD);
}

int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifdef CONFIG_ARCH_ROCKCHIP
	reset_misc();
#endif

	if (argc > 1)
		reboot(argv[1]);
	else
		reboot(NULL);

	return 0;
}

UCLASS_DRIVER(sysreset) = {
	.id		= UCLASS_SYSRESET,
	.name		= "sysreset",
};
