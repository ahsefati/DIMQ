//#include <stdbool.h>
//#include <cstdio>
//#include <stdlib.h>
#include <dimqpp.h>

static int run = -1;

class dimqpp_test : public dimqpp::dimqpp
{
	public:
		dimqpp_test(const char *id);

		void on_connect(int rc);
		void on_disconnect(int rc);
};

dimqpp_test::dimqpp_test(const char *id) : dimqpp::dimqpp(id)
{
}

void dimqpp_test::on_connect(int rc)
{
	if(rc){
		exit(1);
	}else{
		disconnect();
	}
}

void dimqpp_test::on_disconnect(int rc)
{
	run = rc;
}


int main(int argc, char *argv[])
{
	struct dimqpp_test *dimq;

	int port = atoi(argv[1]);

	dimqpp::lib_init();

	dimq = new dimqpp_test("01-con-discon-success");

	dimq->connect("localhost", port, 60);

	while(run == -1){
		dimq->loop();
	}
	delete dimq;

	dimqpp::lib_cleanup();

	return run;
}
