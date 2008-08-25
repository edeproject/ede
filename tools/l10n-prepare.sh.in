#!/bin/sh
# $Id$
# Copy and rename message.pot files, according to their source and pack it
# Inspired with l10n-prepare.pl Vedran Ljubovic wrote for EDE 1.x
#
# Copyright (c) 2005 Vedran Ljubovic
# Copyright (c) 2008 Sanel Zukan
# License as usually, the same as the rest of EDE parts...

pots="ede-2.0.0-l10n"

directories="
 eabout 
 ecalc 
 ecolorconf 
 econtrol 
 ecrasher 
 edesktopconf
 edewm 
 edialog 
 edisplayconf 
 efiler 
 efinder 
 ehelp 
 eiconman 
 eimage
 einstaller 
 ekeyconf 
 elauncher 
 elma 
 emenueditor 
 emountd 
 epanelconf
 esvrconf 
 etimedate 
 etip 
 evoke 
 evolume 
 ewmconf 
 eworkpanel 
 exset"

[ -d $pots ] && rm -R $pots
mkdir $pots

for i in $directories; do
	printf "Processing %-25s" "$i..."
	if [ -r "../$i/locale/messages.pot" ]; then
		cp "../$i/locale/messages.pot" "$pots/$i.pot"
		echo "OK"
	else
		echo "Not found"
	fi
done

cat >$pots/README <<EOF
This directory contains files with untranslated strings from various EDE programs and
the filename with .pot extension matches the program strings was extracted from.

In case you find some spare time, it would be very nice to help us by translating
these files to your language or validate and update existing translations.

Your contribution is very much appreciated. Thank you advaced!!!

EDE Team :-)
http://equinox-project.org
EOF

tar -czpf $pots.tar.gz $pots
rm -R $pots
