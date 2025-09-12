use Config;
use ExtUtils::MakeMaker;

do 'FCGI.cfg' or die "no FCGI.cfg";

open OUT, ">FCGI.pm";

print "Generating FCGI.pm\n";
print OUT <<'EOP';
# $Id: FCGI.PL,v 1.37 2002/12/15 20:02:48 skimo Exp $

package FCGI;

require Exporter;
require DynaLoader;

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(
	
);

EOP

print OUT '$VERSION = '.MM->parse_version('version.pm').";\n\n";

print OUT "bootstrap FCGI;\n" unless ($pure);

print OUT <<'EOP' if ($pure);
use Symbol;
use POSIX 'ENOTCONN';

use constant VERSION_1 => 1;

use constant BEGIN_REQUEST => 1;
use constant PARAMS => 4;
use constant FCGI_STDIN => 5;
use constant FCGI_STDOUT => 6;
use constant FCGI_STDERR => 7;

use constant RESPONDER => 1;
use constant AUTHORIZER => 2;
use constant FILTER => 3;

%FCGI::rolenames = (RESPONDER, "RESPONDER",
		      AUTHORIZER, "AUTHORIZER",
		      FILTER, "FILTER",
		     );

# This only works on Unix; anyone familiar with Windows is welcome
# to give a hand here
sub IsFastCGI {
    my ($req) = @_;
    $req->{isfastcgi} =
	(!defined getpeername shift->{listen_sock}) && $! == ENOTCONN
	unless exists $req->{isfastcgi};
    return $req->{isfastcgi};
}

sub GetEnvironment {
    return shift->{'env'};
}

sub read_nv_len {
    my ($stream) = @_;
    my $buf;
    return undef unless read $stream, $buf, 1, 0;
    my ($len) = unpack("C", $buf);
    if ($len & 0x80) {
	$buf = pack("C", $len & 0x7F);
	return undef unless read $stream, $buf, 3, 1;
	$len = unpack("N", $buf);
    }
    $len;
}

sub RequestX {
    my $self = {
	in => shift,
	out => shift,
	err => shift,
	env => shift,
	socket => shift,
	flags => shift,
	last => 0,
    };
    open $self->{listen_sock}, "<&=0";
    bless $self, "FCGI";
}

my $run_once = 0;

sub Accept {
    my ($req) = @_;

    unless ($req->IsFastCGI()) {
	return -1 if $run_once;

	$run_once = 1;
	return 0;
    }
    $req->Finish();
    $req->{socket} = gensym();
    if ($req->{last} || !accept($req->{socket}, $req->{listen_sock})) {
	$req->{error} = "accept";
	return -1;
    }
    my ($type, $id, $body) = $req->read_record();
    if ($type != BEGIN_REQUEST) {
	$req->{error} = "begin request";
	return -1;
    }
    my ($role, $flags) = unpack("nC", $body);
    $req->{role} = $role;
    $req->{flags} = $flags;
    $req->{id} = $id;

    %{$req->{env}} = ();
    $req->{env}{FCGI_ROLE} = $FCGI::rolenames{$req->{role}};
    my $param = FCGI::Stream->new($req, PARAMS);
    my ($nlen, $vlen);
    while (defined($nlen = read_nv_len($param)) && 
	   defined($vlen = read_nv_len($param))) {
	my ($name, $val);
	read $param, $name, $nlen;
	read $param, $val, $vlen;
	$req->{env}{$name} = $val;
    }
    $req->Bind;
    $req->{accepted} = 1;

    return 0;
}

sub UndoBindings {
    my ($req) = @_;
    untie ${$req->{in}};
    untie ${$req->{out}};
    untie ${$req->{err}};
    $req->{bound} = 0;
}

sub Bind {
    my ($req) = @_;
    tie ${$req->{in}}, 'FCGI::Stream', $req, FCGI_STDIN;
    tie ${$req->{out}}, 'FCGI::Stream', $req, FCGI_STDOUT;
    tie ${$req->{err}}, 'FCGI::Stream', $req, FCGI_STDERR;
    $req->{bound} = 1;
}

sub Attach {
    my ($req) = @_;
    $req->Bind() if ($req->{accepted} && !$req->{bound});
}

sub Detach {
    my ($req) = @_;
    $req->UndoBindings() if ($req->{accepted} && $req->{bound});
}

sub Finish {
    my ($req) = @_;
    return unless $req->{accepted};
    if ($req->{bound}) {
	$req->UndoBindings();
	# apparently these are harmful
	# close ${$req->{out}};
	# close ${$req->{err}};
    }
    $req->{accepted} = 0;
}

sub LastCall {
    shift->{last} = 1;
}

sub DESTROY {
    shift->Finish();
}

sub read_record {
    my ($self) = @_;
    my ($header, $body);

    read($self->{socket}, $header, 8);
    my ($version, $type, $id, $clen, $plen) = unpack("CCnnC", $header);
    read($self->{socket}, $body, $clen+$plen);
    $body = undef if $clen == 0;
    ($type, $id, $body);
}

sub read {
    my ($self, $rtype, $len) = @_;
    while (length $self->{buf} < $len) {
	my ($type, $id, $buf) = $self->read_record();
	return undef unless defined $buf;
	if ($type != $rtype) {
	    $self->{error} = "unexpected stream type";
	    return 0;
	}
	$self->{buf} .= $buf;
    }
    my ($newbuf, $result) = (substr($self->{buf}, $len), 
			     substr($self->{buf}, 0, $len));
    $self->{buf} = $newbuf;
    $result;
}

sub Flush {
    my ($req) = @_;
}

sub write {
    my ($self, $type, $content, $len) = @_;
    return unless $len > 0;
    $self->write_record($type, $content, $len);
}

sub write_record {
    my ($self, $type, $content, $length) = @_;
    my $offset = 0;
    while ($length > 0) {
	my $len = $length > 32*1024 ? 32*1024 : $length;
	my $padlen = (8 - ($len % 8)) % 8;
	my $templ = "CCnnCxa${len}x$padlen";
	my $data = pack($templ, 
			VERSION_1, $type, $self->{id}, $len, $padlen, 
			substr($content, $offset, $len));
	syswrite $self->{socket}, $data;
	$length -= $len;
	$offset += $len;
    }
}

{ package FCGI::Stream;

sub new {
    my ($class, $src, $type) = @_;
    my $handle = do { \local *FH };
    tie($$handle, $class, $src, $type);
    $handle;
}

sub TIEHANDLE {
    my ($class, $src, $type) = @_;
    bless { src => $src, type => $type }, $class;
}

sub READ {
    my ($stream, undef, $len, $offset) = @_;
    my ($ref) = \$_[1];
    my $buf = $stream->{src}->read($stream->{type}, $len);
    return undef unless defined $buf;
    substr($$ref, $offset, 0, $buf);
    length $buf;
}

sub PRINT {
    my ($stream) = shift;
    for (@_) {
	$stream->{src}->write($stream->{type}, $_, length($_));
    }
}

sub CLOSE {
    my ($stream) = @_;
    $stream->{src}->write_record($stream->{type}, undef, 0);
}

}

EOP
print OUT while <DATA>;
close OUT;
__END__

# Preloaded methods go here.

# Autoload methods go after __END__, and are processed by the autosplit program.

*FAIL_ACCEPT_ON_INTR = sub() { 1 };

sub Request(;***$*$) {
    my @defaults = (\*STDIN, \*STDOUT, \*STDERR, \%ENV, 0, 0);
    $_[4] = fileno($_[4]) if defined($_[4]) && defined(fileno($_[4]));
    splice @defaults,0,@_,@_;
    RequestX(@defaults);
}

sub accept() {
    warn "accept called as a method; you probably wanted to call Accept" if @_;
    if (defined %FCGI::ENV) {
	%ENV = %FCGI::ENV;
    } else {
	%FCGI::ENV = %ENV;
    }
    my $rc = Accept($global_request);
    for (keys %FCGI::ENV) {
	$ENV{$_} = $FCGI::ENV{$_} unless exists $ENV{$_};
    }

    # not SFIO
    $SIG{__WARN__} = $warn_handler if (tied (*STDIN));
    $SIG{__DIE__} = $die_handler if (tied (*STDIN));

    return $rc;
}

sub finish() {
    warn "finish called as a method; you probably wanted to call Finish" if @_;
    %ENV = %FCGI::ENV if (defined %FCGI::ENV);

    # not SFIO
    if (tied (*STDIN)) {
	delete $SIG{__WARN__} if ($SIG{__WARN__} == $warn_handler);
	delete $SIG{__DIE__} if ($SIG{__DIE__} == $die_handler);
    }

    Finish ($global_request);
}

sub flush() {
    warn "flush called as a method; you probably wanted to call Flush" if @_;
    Flush($global_request);
}

sub detach() {
    warn "detach called as a method; you probably wanted to call Detach" if @_;
    Detach($global_request);
}

sub attach() {
    warn "attach called as a method; you probably wanted to call Attach" if @_;
    Attach($global_request);
}

# deprecated
sub set_exit_status {
}

sub start_filter_data() {
    StartFilterData($global_request);
}

$global_request = Request();
$warn_handler = sub { print STDERR @_ };
$die_handler = sub { print STDERR @_ unless $^S };

package FCGI::Stream;

sub PRINTF {
  shift->PRINT(sprintf(shift, @_));
}

sub BINMODE {
}

sub READLINE {
    my $stream = shift;
    my ($s, $c);
    my $rs = $/ eq '' ? "\n\n" : $/;
    my $l = substr $rs, -1;
    my $len = length $rs;

    $c = $stream->GETC();
    if ($/ eq '') {
	while ($c eq "\n") { 
	    $c = $stream->GETC();
	}
    }
    while (defined $c) {
	$s .= $c;
	last if $c eq $l and substr($s, -$len) eq $rs;
	$c = $stream->GETC();
    }
    $s;
}

sub OPEN {
    $_[0]->CLOSE;
    if (@_ == 2) {
	return open($_[0], $_[1]);
    } else {
	my $rc;
	eval("$rc = open($_[0], $_[1], $_[2])");
	die $@ if $@;
	return $rc;
    }
}

# Apparently some use fileno to determine if a filehandle is open,
# so we might want to return a defined, but meaningless value.
# An alternative would be to return the fcgi stream fd.
# sub FILENO { -2 }

1;

=pod

=head1 NAME

FCGI - Fast CGI module

=head1 SYNOPSIS

    use FCGI;

    my $count = 0;
    my $request = FCGI::Request();

    while($request->Accept() >= 0) {
	print("Content-type: text/html\r\n\r\n", ++$count);
    }

=head1 DESCRIPTION

Functions:

=over 4

=item FCGI::Request

Creates a request handle. It has the following optional parameters:

=over 8

=item input perl file handle (default: \*STDIN)

=item output perl file handle (default: \*STDOUT)

=item error perl file handle (default: \*STDERR)

These filehandles will be setup to act as input/output/error
on succesful Accept.

=item environment hash reference (default: \%ENV)

The hash will be populated with the environment.

=item socket (default: 0)

Socket to communicate with the server.
Can be the result of the OpenSocket function.
For the moment, it's the file descriptor of the socket
that should be passed. This may change in the future.

You should only use your own socket if your program
is not started by a process manager such as mod_fastcgi
(except for the FastCgiExternalServer case) or cgi-fcgi. 
If you use the option, you have to let your FastCGI
server know which port (and possibly server) your program
is listening on.
See remote.pl for an example.

=item flags (default: 0)

Possible values:

=over 12

=item FCGI::FAIL_ACCEPT_ON_INTR

If set, Accept will fail if interrupted.
It not set, it will just keep on waiting.

=back

=back

Example usage:
    my $req = FCGI::Request;

or:
    my %env;
    my $in = new IO::Handle;
    my $out = new IO::Handle;
    my $err = new IO::Handle;
    my $req = FCGI::Request($in, $out, $err, \%env);

=item FCGI::OpenSocket(path, backlog)

Creates a socket suitable to use as an argument to Request.

=over 8

=item path

Pathname of socket or colon followed by local tcp port.
Note that some systems take file permissions into account
on Unix domain sockets, so you'll have to make sure that
the server can write to the created file, by changing
the umask before the call and/or changing permissions and/or
group of the file afterwards.

=item backlog

Maximum length of the queue of pending connections.
If a connection
request arrives with the queue full the client may receive
an  error  with  an  indication of ECONNREFUSED.

=back

=item FCGI::CloseSocket(socket)

Close a socket opened with OpenSocket.

=item $req->Accept()

Accepts a connection on $req, attaching the filehandles and
populating the environment hash.
Returns 0 on success.
If a connection has been accepted before, the old
one will be finished first.

Note that unlike with the old interface, no die and warn
handlers are installed by default. This means that if
you are not running an sfio enabled perl, any warn or
die message will not end up in the server's log by default.
It is advised you set up die and warn handlers yourself.
FCGI.pm contains an example of die and warn handlers.

=item $req->Finish()

Finishes accepted connection.
Also detaches filehandles.

=item $req->Flush()

Flushes accepted connection.

=item $req->Detach()

Temporarily detaches filehandles on an accepted connection.

=item $req->Attach()

Re-attaches filehandles on an accepted connection.

=item $req->LastCall()

Tells the library not to accept any more requests on this handle.
It should be safe to call this method from signal handlers.

Note that this method is still experimental and everything
about it, including its name, is subject to change.

=item $env = $req->GetEnvironment()

Returns the environment parameter passed to FCGI::Request.

=item ($in, $out, $err) = $req->GetHandles()

Returns the file handle parameters passed to FCGI::Request.

=item $isfcgi = $req->IsFastCGI()

Returns whether or not the program was run as a FastCGI.

=back

=head1 AUTHOR

Sven Verdoolaege <skimo@kotnet.org>

=cut

__END__
