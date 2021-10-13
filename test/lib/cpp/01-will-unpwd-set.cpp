#include <cstring>
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

	int port = atoi(argv[1]);

	dimqpp::lib_init();

	dimq = new dimqpp_test("01-will-unpwd-set");
	dimq->username_pw_set("oibvvwqw", "#'^2hg9a&nm38*us");
	dimq->will_set("will-topic", strlen("will message"), "will message", 2, false);

	dimq->connect("localhost", port, 60);

	while(run == -1){
		dimq->loop();
	}
	delete dimq;

	delete dimq;
	dimqpp::lib_cleanup();

	return run;
}
