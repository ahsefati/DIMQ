#include <cstring>
#include <dimqpp.h>

static int run = -1;

static int password_callback(char* buf, int size, int rwflag, void* userdata)
{
	strncpy(buf, "password", size);
	buf[size-1] = '\0';

	return strlen(buf);
}

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

	dimq = new dimqpp_test("08-ssl-connect-crt-auth-enc");

	dimq->tls_opts_set(1, "tlsv1", NULL);
	//dimq->tls_set("../ssl/test-ca.crt", NULL, "../ssl/client.crt", "../ssl/client.key");
	dimq->tls_set("../ssl/all-ca.crt", NULL, "../ssl/client-encrypted.crt", "../ssl/client-encrypted.key", password_callback);
	dimq->connect("localhost", port, 60);

	while(run == -1){
		dimq->loop();
	}
	delete dimq;

	delete dimq;
	dimqpp::lib_cleanup();

	return run;
}
