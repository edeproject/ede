#!/bin/sh
# download fresh repository copy of ede and edelib and do whatever magic is needed
# to prepare release

RELEASE_VERSION="2.0"

############################################

dprint() {
	echo "* $1"
}

release_dir="release-$RELEASE_VERSION"
release_dir_full="`pwd`/$release_dir"

# prepare location dir
rm -Rf $release_dir
mkdir -p $release_dir
cd $release_dir

ede_svn_base="https://ede.svn.sourceforge.net/svnroot/ede"
ede_svn_trunk="$ede_svn_base/trunk"
ede_svn_tags="$ede_svn_base/tags"

############################################

# make tag(s)
dprint "Tagging edelib"
svn copy "$ede_svn_trunk/edelib" "$ede_svn_tags/edelib-$RELEASE_VERSION" -m "Tagging edelib to $RELEASE_VERSION"

dprint "Tagging ede"
svn copy "$ede_svn_trunk/ede2"   "$ede_svn_tags/ede-$RELEASE_VERSION" -m "Tagging ede to $RELEASE_VERSION"

############################################

package="edelib-$RELEASE_VERSION"

dprint "Getting edelib..."
svn co "$ede_svn_trunk/edelib" $package

cd $package
find . -name ".svn" -type d | xargs rm -Rf
./autogen.sh
rm -Rf "autom4te.cache"
cd ..

tar -czpvf $package.tar.gz $package
md5sum $package.tar.gz > checksum

############################################

package="ede-$RELEASE_VERSION"

dprint "Getting ede..."
svn co "$ede_svn_trunk/ede2" $package

cd $package
find . -name ".svn" -type d | xargs rm -Rf
./autogen.sh
rm -Rf "autom4te.cache"

# make icon theme
cd data/icon-themes
mv edeneu edeneu-svg
edelib-convert-icontheme edeneu-svg edeneu
edelib-mk-indextheme edeneu > edeneu/index.theme
cd ..

cd $release_dir_full
cd $package

# append it
echo "# added by $0" >> Jamconfig.in
echo "INSTALL_ICON_THEMES = 1 ;" >> Jamconfig.in
cd ..

tar -czpvf $package.tar.gz $package
md5sum $package.tar.gz >> checksum

