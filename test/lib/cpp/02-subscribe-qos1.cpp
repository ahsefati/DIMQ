#include <dimqpp.h>

static int run = -1;

class dimqpp_test : public dimqpp::dimqpp
{
	public:
		dimqpp_test(const char *id);

		void on_connect(int rc);
		void on_disconnect(int rc);
		void on_subscribe(int mid, int qos_count, const int *granted_qos);
};

dimqpp_test::dimqpp_test(const char *id) : dimqpp::dimqpp(id)
{
}

void dimqpp_test::on_connect(int rc)
{
	if(rc){
		exit(1);
	}else{
		subscribe(NULL, "qos1/test", 1);
	}
}

void dimqpp_test::on_disconnect(int rc)
{
	run = rc;
}

void dimqpp_test::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
	disconnect();
}


int main(int argc, char *argv[])
{
	struct dimqpp_test *dimq;

	int port = atoi(argv[1]);

	dimqpp::lib_init();

	dimq = new dimqpp_test("subscribe-qos1-test");

	dimq->connect("localhost", port, 60);

	while(run == -1){
		dimq->loop();
	}

	delete dimq;
	dimqpp::lib_cleanup();

	return run;
}
