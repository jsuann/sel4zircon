#include "object/job.h"

namespace JobCxx {
    ZxJob *root_job;
};

void init_root_job()
{
    using namespace JobCxx;
    root_job = allocate_object<ZxJob>();
    assert(root_job != NULL);
}

ZxJob *get_root_job()
{
    return JobCxx::root_job;
}
