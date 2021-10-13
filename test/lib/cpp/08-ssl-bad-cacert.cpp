#include <dimqpp.h>

static int run = -1;

class dimqpp_test : public dimqpp::dimqpp
{
	public:
		dimqpp_test(const char *id);
};

dimqpp_test::dimqpp_test(const char *id) : dimqpp::dimqpp(id)
{
}

int main(int argc, char *argv[])
{
	struct dimqpp_test *dimq;
	int rc = 1;

	dimqpp::lib_init();

	dimq = new dimqpp_test("08-ssl-bad-cacert");

	dimq->tls_opts_set(1, "tlsv1", NULL);
	if(dimq->tls_set("this/file/doesnt/exist") == dimq_ERR_INVAL){
		rc = 0;
	}
	delete dimq;
	dimqpp::lib_cleanup();

	return rc;
}
