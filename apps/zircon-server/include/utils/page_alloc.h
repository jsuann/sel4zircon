#pragma once

#include <autoconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void init_page_alloc(vka_t *vka);
uint32_t get_num_page_avail();
void *page_alloc();
void *page_alloc_zero();
void page_free(void *page);
