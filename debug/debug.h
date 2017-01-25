#ifndef PRIVATE_DBG_H_INCLUDE
#define PRIVATE_DBG_H_INCLUDE
#include <stdio.h>
#include "DBG.h"

struct DBG_Controller_tag {
    FILE *debug_write_fp;
    int current_debug_level;
};

#endif
