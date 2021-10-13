#include <cstring>

#include <dimqpp.h>

static int run = -1;
static int sent_mid = -1;

class dimqpp_test : public dimqpp::dimqpp
{
	public:
		dimqpp_test(const char *id);

		void on_connect(int rc);
		void on_publish(int mid);
};

dimqpp_test::dimqpp_test(const char *id) : dimqpp::dimqpp(id)
{
}

void dimqpp_test::on_connect(int rc)
{
	if(rc){
		exit(1);
	}else{
		publish(&sent_mid, "pub/qos0/no-payload/test", 0, NULL, 0, false);
	}
}

void dimqpp_test::on_publish(int mid)
{
	if(sent_mid == mid){
		disconnect();
	}else{
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	struct dimqpp_test *dimq;

	int port = atoi(argv[1]);

	dimqpp::lib_init();

	dimq = new dimqpp_test("publish-qos0-test-np");

	dimq->connect("localhost", port, 60);

	while(run == -1){
		dimq->loop();
	}
	delete dimq;

	delete dimq;
	dimqpp::lib_cleanup();

	return run;
}
