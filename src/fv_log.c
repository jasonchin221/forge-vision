#include "fv_types.h"
#include "fv_log.h"

fv_bool fv_log_on = 0;

void
fv_log_enable(void)
{
    fv_log_on = 1;
}

void
fv_log_disable(void)
{
    fv_log_on = 0;
}
