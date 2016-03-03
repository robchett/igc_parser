#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "convert.h"

#if !defined(M_PI)
#define M_PI 3.14159265358979323846
#endif

#ifdef __cplusplus
#define NEW(type, count) ((type *)calloc(count, sizeof(type)))
#else
#define NEW(type, count) (calloc(count, sizeof(type)))
#endif