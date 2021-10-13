#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <dimq.h>

int main(int argc, char *argv[])
{
	int rc = 1;
	struct dimq *dimq;

	dimq_lib_init();

	dimq = dimq_new("08-ssl-bad-cacert", true, NULL);
	if(dimq == NULL){
		return 1;
	}
	if(dimq_tls_set(dimq, "this/file/doesnt/exist", NULL, NULL, NULL, NULL) == dimq_ERR_INVAL){
		rc = 0;
	}
	dimq_destroy(dimq);
	dimq_lib_cleanup();
	return rc;
}
