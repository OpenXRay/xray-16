use Config;

open OUT, ">threaded.fpl";
print OUT "#!$Config{perlpath}\n";
print OUT while <DATA>;
close OUT;
chmod 0755, "threaded.fpl";
__END__

use FCGI;
use Thread;
use IO::Handle;

use constant THREAD_COUNT => 5;

sub doit {
    my $k = shift;
    my %env;
    my $in = new IO::Handle;
    my $out = new IO::Handle;
    my $err = new IO::Handle;

    my $request = FCGI::Request($in, $out, $err, \%env);

    while ($request->Accept() >= 0) {
	print $out
           "Content-type: text/html\r\n",
           "\r\n",
           "<title>FastCGI Hello! (multi-threaded perl, fcgiapp library)</title>",
           "<h1>FastCGI Hello! (multi-threaded perl, fcgiapp library)</h1>",
           "Request counts for ", THREAD_COUNT ," threads ",
	   "running on host <i>$env{SERVER_NAME}</i><P><CODE>";

	{
	    lock(@count);

	    ++$count[$k];

	    for(my $i = 0; $i < THREAD_COUNT; ++$i) {
		print $out $count[$i];
		print $out " ";
	    }
	}
	$request->Flush();
	sleep(1);
    }
}

for ($t = 1; $t < THREAD_COUNT; ++$t) {
    new Thread \&doit, $t;
}
doit(0);
