#!/usr/local/bin/perl

#
# Files used
$RAD_FILE   = "./users";
$OUTPUT     = "./users.out";
$CMDS       = "./cmds";

#
# misc.
$DEF_FA = "255.255.255.254";
@entry  = @fa = @tmp = ();
$pvuser = "";
$line   = "";
$user   = "";
$pw     = "";
$proc   = 0;

@USERADD = ('/usr/local/admin/useradd', 'user', '"name"');
@STDINPW = ('/bin/echo', 'pw', '|', '/usr/local/admin/stdinpasswd', '-i', 'user');

@def = (
	"#\n",
	"DEFAULT Password = \"UNIX\"\n",
	"\tUser-Service-Type = Framed-User,\n",
	"\tFramed-Protocol = PPP,\n",
	"\tFramed-Address = 255.255.255.254,\n",
	"\tFramed-Netmask = 255.255.255.0,\n",
	"\tFramed-Routing = Broadcast,\n",
	"\tFramed-Compression = Van-Jacobsen-TCP-IP,\n",
	"\tFramed-MTU = 1500\n"
	);

main();

sub main {
        $| = 0;
        open FH, "$RAD_FILE"    || die "open: $!";
        open OUT, "> $OUTPUT"   || die "open: $!";
	open CMD, "> $CMDS"	|| die "open: $!";

	print CMD "#!/bin/sh\n";

        while ($line = <FH>) {
                if ($line =~ /^\#$/) {
                        next;
                }

		check_user ();

                #
                # Indicates new radius entry
                if ($line =~ /^[a-zA-Z]/) {
                        #
                        # Set after the first entry has been processed.
                        if ($proc) {
				p_stuff();
			}
			$pvuser = $user;
			$proc = 1;
		}
		push @entry, $line;
	}

	if ($proc) {
		p_stuff();
	}

	print OUT @def;

	close FH;
	close OUT;
	close CMD;

	chmod 0755, $CMDS;

	exit 0;
}

sub p_stuff {
	#
	# Entry is a user without a password, go onto
	# next entry.
	grep {
		/^(.*password[\s]*=[\s]*")([^"]*)(".*)$/i && do {
			$pw = $2;
			($_ = "$1UNIX$3\n");
		};
	} @entry;

	#
	# if the account has been hosed, forget it.
	if (	$pw =~ /^\#\#/ 		or 
		$pw =~ /CANCEL/i 	or
		$pw =~ /PAUSED/i	or
		$pw =~ /DISABLE/i	or
		$pw =~ /HOLD/		or
		$pw =~ /LOCK/
	) {
		@entry = ();
		return;
	}

	@fa = grep (/Framed-Address/, @entry);

	#
	# Specialized entries, such as static IP's
	# and Cisco IP pool assignments need individual
	# radius entries.  Everyone else falls under
	# DEFAULT template.
	if ($#fa >= 0) {
		if ($fa[0] !~ /$DEF_FA/) {
			print OUT "\#\n";
			print OUT @entry;
		}
	} elsif (grep (/Cisco-AVPair/, @entry)) {
		print OUT "\#\n";
		print OUT @entry;
	}

	setup_login();
	@entry = ();
	return;
}

sub setup_login {
	local $cmd = '';
	
	print CMD "echo \"$pvuser\"\n";
	@USERADD[1] = $pvuser;
	@USERADD[2] = '"Radius Login"';

	$cmd = join (' ', @USERADD);
	print CMD "$cmd\n";

	@STDINPW[1] = "'" . $pw . "'";
	@STDINPW[5] = $pvuser;

	$cmd = join (' ', @STDINPW);
	print CMD "$cmd\n";
	return;
}

sub check_user {
	@tmp = split(/[ \t]/, $line);
	$user = @tmp[0];
	#
	# fix invalid usernames
	if (length ($user) > 8) {
		($line =~ /^(\S+)(.*)$/) && do {
			$line = substr($1, 0, 8) . "$2";
		};
		$user = substr($user, 0, 8);
	}
	if ($user =~ /[^a-zA-Z0-9_]+/) {
		$user =~ s/[^a-zA-Z0-9_]//g;
		($line =~ /^(\S+)(.*)$/) && do {
			$line = $user . $2;
		}
	}
	if ($user =~ /^P[a-zA-Z0-9_]+/) {
		$user =~ s/^P//;
		($line =~ /^(\S+)(.*)$/) && do {
			$line = $user . $2;
		}
	}
}

