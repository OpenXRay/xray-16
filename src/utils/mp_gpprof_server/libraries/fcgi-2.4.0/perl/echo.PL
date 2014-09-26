use Config;

open OUT, ">echo.fpl";
print OUT "#!$Config{perlpath}\n";
print OUT while <DATA>;
close OUT;
chmod 0755, "echo.fpl";
__END__
#
#  echo-perl --
# 
# 	Produce a page containing all FastCGI inputs
# 
# Copyright (c) 1996 Open Market, Inc.
#
# See the file "LICENSE.TERMS" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# 
#  $Id: echo.PL,v 1.2 2000/12/14 13:46:23 skimo Exp $
#
# Changed by skimo to demostrate autoflushing 1997/02/19
#

use FCGI;
use strict;

sub print_env {
    my($label, $envp) = @_;
    print("$label:<br>\n<pre>\n");
    my @keys = sort keys(%$envp);
    foreach my $key (@keys) {
        print("$key=$$envp{$key}\n");
    }
    print("</pre><p>\n");
}

my %env;
my $req = FCGI::Request(\*STDIN, \*STDOUT, \*STDERR, \%env);
my $count = 0;
while($req->Accept() >= 0) {
    print("Content-type: text/html\r\n\r\n",
          "<title>FastCGI echo (Perl)</title>\n",
          "<h1>FastCGI echo (Perl)</h1>\n",
          "Request number ", ++$count, "<p>\n");
    my $len = 0 + $env{'CONTENT_LENGTH'};
    if($len == 0) {
        print("No data from standard input.<p>\n");
    } else {
        print("Standard input:<br>\n<pre>\n");
        for(my $i = 0; $i < $len; $i++) {
            my $ch = getc(STDIN);
            if($ch eq "") {
                print("Error: Not enough bytes received ",
                      "on standard input<p>\n");
                last;
	    }
            print($ch);
        }
        print("\n</pre><p>\n");
    }
    print_env("Request environment", \%env);
    print "More on its way ... wait a few seconds\n<BR>\n<BR>";
    $req->Flush();
    sleep(3);
    print_env("Initial environment", \%ENV);
    $req->Finish();
}
