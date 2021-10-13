#include <errno.h>
#include <dimqpp.h>

static int run = -1;

class dimqpp_test : public dimqpp::dimqpp
{
	public:
		dimqpp_test(const char *id);

		void on_connect(int rc);
};

dimqpp_test::dimqpp_test(const char *id) : dimqpp::dimqpp(id)
{
}

void dimqpp_test::on_connect(int rc)
{
	exit(1);
}

int main(int argc, char *argv[])
{
	struct dimqpp_test *dimq;
	int rc;

	int port = atoi(argv[1]);

	dimqpp::lib_init();

	dimq = new dimqpp_test("08-ssl-fake-cacert");

	dimq->tls_opts_set(1, "tlsv1", NULL);
	dimq->tls_set("../ssl/test-fake-root-ca.crt", NULL, "../ssl/client.crt", "../ssl/client.key");
	dimq->connect("localhost", port, 60);

	rc = dimq->loop_forever();
	delete dimq;
	dimqpp::lib_cleanup();
	if(rc == dimq_ERR_ERRNO && errno == EPROTO){
		return 0;
	}else{
		return 1;
	}
}
