#!/usr/bin/perl

# This Perl script prepares files for translations.
# Extracts strings from code, copies into l10n/ directory and
# creates convinient .tar.gz packages for languages.
# Copyright (c) Vedran Ljubovic 2005.
# This program is licensed under terms of GNU General Public 
# License v2 or greater.


@directories = ("common", "ecolorconf", "econtrol", "edewm", 
"edisplayconf", "efinder","eiconman", "eiconsconf", "einstaller", 
"ekeyconf", "elauncher", "emenueditor", "epanelconf", "esvrconf", 
"etimedate", "etip", "evolume", "ewmconf", "eworkpanel");


foreach $dir (@directories) {
	print "Extracting $dir...\n";

	# Extract new strings from code
	$command = "xgettext -o $dir/locale/messages.pot --keyword=\_ $dir/*.cpp $dir/*.h";
	`$command`;
}


print "Copying files...\n";
while ($nextname = <*/locale/*.po>) {
	if ($nextname =~ /(.*?)\/locale\/(.*?)\.po/) {
		$dest = "l10n/$2/$1".".po";
		`mkdir l10n/$2 &>/dev/null`;
		`cp -f $nextname $dest`;
	} else {
		print "Error: $nextname\n";
	}
}

`mkdir l10n/nontranslated &>/dev/null`;
while ($nextname = <*/locale/messages.pot>) {
	if ($nextname =~ /(.*?)\/locale\/messages.pot/) {
		$dest = "l10n/nontranslated/$1".".pot";
		`cp -f $nextname $dest`;
	} else {
		print "Error: $nextname\n";
	}
}

while ($nextname = <l10n/*>) {
	if ((-d $nextname) && ($nextname =~ /l10n\/(.*?)$/)) {
	print "Creating package for $1...\n";
		`tar czvf l10n/$1.tar.gz l10n/$1`;
	}
}

