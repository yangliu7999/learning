/*
 * Jailhouse, a Linux-based partitioning hypervisor
 *
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Authors:
 *  Lokesh Vutla <lokeshvutla@ti.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#define SMCCC_VERSION			0x80000000
#define SMCCC_ARCH_FEATURES		0x80000001
#define SMCCC_ARCH_WORKAROUND_1		0x80008000
#define SMCCC_ARCH_WORKAROUND_2		0x80007fff

#define ARM_SMCCC_OWNER_MASK		BIT_MASK(29, 24)
#define ARM_SMCCC_OWNER_SHIFT		24

#define ARM_SMCCC_OWNER_ARCH		0
#define ARM_SMCCC_OWNER_SIP             2
#define ARM_SMCCC_OWNER_STANDARD        4

#define ARM_SMCCC_CONV_32		0
#define ARM_SMCCC_CONV_64		1

#define ARM_SMCCC_NOT_SUPPORTED         (-1)
#define ARM_SMCCC_SUCCESS               0

#define ARM_SMCCC_VERSION_1_1		0x10001

#define SMCCC_GET_OWNER(id)	((id & ARM_SMCCC_OWNER_MASK) >>	\
				 ARM_SMCCC_OWNER_SHIFT)

#define SMCCC_IS_CONV_64(function_id)	!!(function_id & (1 << 30))

#ifndef __ASSEMBLY__

struct trap_context;

void smccc_discover(void);
enum trap_return handle_smc(struct trap_context *ctx);

#endif /* !__ASSEMBLY__ */
