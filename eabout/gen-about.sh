#!/bin/sh

echo 'const char about_content[] ='
sed -e '/^Authors/d'   \
    -e '/=\{3,\}/d'   \
    -e 's/-\{3,\}//'  \
    -e 's/\(.*\)/    " \1\\n"/'
echo ';'
