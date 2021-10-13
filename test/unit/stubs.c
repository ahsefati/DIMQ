#include "config.h"

#include <time.h>
#include <logging_dimq.h>

struct dimq_db{

};

int log__printf(struct dimq *dimq, unsigned int priority, const char *fmt, ...)
{
	UNUSED(dimq);
	UNUSED(priority);
	UNUSED(fmt);

	return 0;
}

time_t dimq_time(void)
{
	return 123;
}

int net__socket_close(struct dimq_db *db, struct dimq *dimq)
{
	UNUSED(db);
	UNUSED(dimq);

	return dimq_ERR_SUCCESS;
}

int send__pingreq(struct dimq *dimq)
{
	UNUSED(dimq);

	return dimq_ERR_SUCCESS;
}

