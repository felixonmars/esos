/*
 * Copyright (c) 2022-2025, Spacemit
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define REGULATOR_DESC_TEST(_id, _match, _lr)			\
	{							\
		.name			= (_match),		\
		.id			= (_id),		\
		.linear_ranges		= (_lr),		\
		.n_linear_ranges	= ARRAY_SIZE(_lr)	\
	}

#define REGULATOR_LINEAR_RANGE(_min_uV, _max_uV, _step_uV)	\
{								\
	.min	= _min_uV,					\
	.max	= _max_uV,					\
	.step	= _step_uV,					\
}

struct linear_range {
	unsigned int min;
	unsigned int max;
	unsigned int step;
};

struct voltage_test {
	const char *name;
	int id;
	const struct linear_range *linear_ranges;
	int n_linear_ranges;
};

static const struct linear_range test_ranges[] = {
	REGULATOR_LINEAR_RANGE(500000, 3400000, 25000),
};

static const struct voltage_test test_reg[] = {
	REGULATOR_DESC_TEST(0, "regulator-test", test_ranges),
};
