/*
 * Copyright (c) 2020 Louis Suen
 * Licensed under the MIT License. See the LICENSE file for the full text.
 */
#ifndef __LOG__H__D76B1C4F_4C55_4497_AA37_CE482DC90E19
#define __LOG__H__D76B1C4F_4C55_4497_AA37_CE482DC90E19

#include <stdio.h>

#define LOGD(format, ...) fprintf(stdout, format, ##__VA_ARGS__)
#define LOGW(format, ...) fprintf(stdout, format, ##__VA_ARGS__)
#define LOGE(format, ...) fprintf(stderr, format, ##__VA_ARGS__)

#endif

