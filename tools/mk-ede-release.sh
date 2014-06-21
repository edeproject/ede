#!/bin/sh
# download fresh repository copy of ede and edelib and do whatever magic is needed
# to prepare release

RELEASE_VERSION="2.1"

############################################

script_dir="$( cd "$( dirname "$0" )" && pwd )"
release_dir="release-$RELEASE_VERSION"
release_dir_full="`pwd`/$release_dir"

dprint() {
	echo "* $1"
}

generate_changelog() {
	$script_dir/svn2cl/svn2cl.sh --authors=$script_dir/svn2cl/authors.xml -o $1/ChangeLog $1
}

upload_file() {
	scp $1-$2.tar.gz karijes@frs.sourceforge.net:/home/frs/project/ede/$1/$2/$1-$2.tar.gz
}

# prepare location dir
rm -Rf $release_dir
mkdir -p $release_dir
cd $release_dir

ede_svn_base="https://svn.code.sf.net/p/ede/code"
ede_svn_trunk="$ede_svn_base/trunk"
ede_svn_tags="$ede_svn_base/tags"

############################################

# make tag(s)
dprint "Tagging edelib"
svn copy "$ede_svn_trunk/edelib" "$ede_svn_tags/edelib-$RELEASE_VERSION" -m "Tagging edelib to $RELEASE_VERSION"

dprint "Tagging ede"
svn copy "$ede_svn_trunk/ede2"   "$ede_svn_tags/ede-$RELEASE_VERSION" -m "Tagging ede to $RELEASE_VERSION"

###########################################

package="edelib-$RELEASE_VERSION"

dprint "Getting edelib..."
svn co "$ede_svn_trunk/edelib" $package

generate_changelog $package

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

generate_changelog $package

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

############################################

#upload_file edelib $RELEASE_VERSION
#upload_file ede $RELEASE_VERSION
