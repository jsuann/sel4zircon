#include "utils/system.h"

namespace SystemCxx {

uint64_t tsc_freq;
uint32_t num_cores;
uint64_t phys_mem;

} /* namespace SystemCxx */

void init_system_info(env_t *env)
{
    using namespace SystemCxx;

    /* we have ticks per second syscall, so
       convert tsc freq from MHz to Hz */
    tsc_freq = env->tsc_freq * 1000000;
    num_cores = env->num_cores;
    phys_mem = env->phys_mem;

    dprintf(INFO, "tsc freq: %lu\n", tsc_freq);
    dprintf(INFO, "num cores: %u\n", num_cores);
    //dprintf(INFO, "phys mem: %lx\n", phys_mem);
}

uint64_t system_get_tsc_freq()
{
    return SystemCxx::tsc_freq;
}

uint32_t system_get_num_cores()
{
    return SystemCxx::num_cores;
}
