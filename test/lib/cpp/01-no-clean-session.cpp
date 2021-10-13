#include <cstring>
#include <dimqpp.h>

static int run = -1;

class dimqpp_test : public dimqpp::dimqpp
{
	public:
		dimqpp_test(const char *id, bool clean_session);
};

dimqpp_test::dimqpp_test(const char *id, bool clean_session) : dimqpp::dimqpp(id, clean_session)
{
}

int main(int argc, char *argv[])
{
	struct dimqpp_test *dimq;

	int port = atoi(argv[1]);

	dimqpp::lib_init();

	dimq = new dimqpp_test("01-no-clean-session", false);

	dimq->connect("localhost", port, 60);

	while(run == -1){
		dimq->loop();
	}
	delete dimq;

	delete dimq;
	dimqpp::lib_cleanup();

	return run;
}
