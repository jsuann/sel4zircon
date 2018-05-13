#include "utils/system.h"

namespace SystemCxx {

uint64_t tsc_freq;
uint64_t num_cores;
uint64_t phys_mem;

} /* namespace SystemCxx */

void init_system_info(env_t *env)
{
    using namespace SystemCxx;

    tsc_freq = env->tsc_freq;
    num_cores = env->num_cores;
    phys_mem = env->phys_mem;

    dprintf(INFO, "tsc freq: %lu\n", tsc_freq);
    dprintf(INFO, "num cores: %lu\n", num_cores);
    dprintf(INFO, "phys mem: %lx (%lu MB)\n", phys_mem, (phys_mem / (1024 * 1024)));
}
