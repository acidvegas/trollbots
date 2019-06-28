#!/usr/bin/perl

use Net::DNS;
  
my $separator=:;
my $res=Net::DNS::Resolver->new;

while (<>){
	chomp;
	my ($host,$port) = split(/$separator/o,$_);
	unless ($host=~m/^\d+\.\d+\.\d+\.\d+$/o) {
		my $dnspack = $res->query($host,A);
		if ($dnspack){
			foreach my $rdata ($dnspack->answer){
				my $rstr=$rdata->rdatastr;
				print "$rstr$separator$port\n";
			}
		} else {
			print "$host$separator$port\n";
		}
	} else {
		print "$host$separator$port\n";
	}
}

__END__

=head1 NAME

dns2ip - convert proxy list from HOST:PORT fromat to IP:PORT format

=head1 SYNOPSIS

dns2ip < host_port.txt > ip_port.txt
