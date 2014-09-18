use Config;

open OUT, ">remote.fpl";
print OUT "#!$Config{perlpath}\n";
print OUT while <DATA>;
close OUT;
chmod 0755, "remote.fpl";
__END__
# An example of using a remote script with an Apache webserver.
# Run this Perl program on "otherhost" to bind port 8888 and wait
# for FCGI requests from the webserver.

## Sample Apache configuration on the webserver to refer to the
## remote script on "otherhost"
# <IFModule mod_fastcgi.c>
#  AddHandler fastcgi-script fcgi
#  FastCgiExternalServer /path-to/cgi-bin/external.fcgi -host otherhost:8888
# </IfModule>

# Access the URL:  http://webserver/cgi-bin/external.fcgi

# Contributed by Don Bindner <dbindner@truman.edu>

use FCGI;

my $socket = FCGI::OpenSocket( ":8888", 5 );
my $request = FCGI::Request( \*STDIN, \*STDOUT, \*STDERR,
    \%ENV, $socket );

my $count;
while( $request->Accept() >= 0 ) {
    print "Content-type: text/html\r\n\r\n";
    print ++$count;
}

FCGI::CloseSocket( $socket );
