#include <iostream>
#include <fstream>
#include <memory>

#include <fcgi_config.h>
#include <fcgio.h>
#include <fcgiapp.h>
#include <pthread.h>

#include "sake_worker.h"
#include "requests_processor.h"
#include "profile_request.h"
#include "profile_printer.h"

char const *	game_name			= "stalkercoppc";
int				game_id				= 2760;
int				game_product_id		= 11994;
int				game_namespace_id	= 1;

char const * default_root_path	= "/gprof";
char const * root_path			= NULL;

int main(int argc, char** argv)
{
	using namespace std;
	using namespace gamespy_profile;
	
	try
	{

#ifdef WIN32
	pthread_win32_process_attach_np();
#endif

	
	/*streambuf*		cin_streambuf  = cin.rdbuf();
    streambuf*		cout_streambuf = cout.rdbuf();
    streambuf*		cerr_streambuf = cerr.rdbuf();*/
	
	FCGX_Init();
	int binded_sock = 0;
	if (argc >= 2)
		binded_sock = FCGX_OpenSocket(argv[1], 128);
	if (!binded_sock)
	{
		cerr << "failed to bind socket to address " << argv[1] << endl;
		return EXIT_FAILURE;
	}

	cout << "binded server to: " << argv[1] << ", value=" << binded_sock << endl;

	if (argc >= 3)
		root_path = argv[2];
		
	std::auto_ptr<requests_poll>			req_poll(new requests_poll());
	fetch_profile_request::request_ptr_t	new_request(new FCGX_Request());

	FCGX_InitRequest(new_request.get(), binded_sock, 0);

	cout << "listening requests..." << endl;
	while (FCGX_Accept_r(new_request.get()) == 0)
	{
		req_poll->add_request(new_request);
		assert(!new_request.get());
		new_request.reset(new FCGX_Request());
		FCGX_InitRequest(new_request.get(), binded_sock, 0);
	}

	/*cin.rdbuf(cin_streambuf);
    cout.rdbuf(cout_streambuf);
    cerr.rdbuf(cerr_streambuf);*/
	
	} catch (std::exception const & e)
	{
		cerr << "Caught exception: " << e.what() << endl
			<< "Type: " << typeid(e).name() << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
};
