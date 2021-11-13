#!/bin/sh
# Run this to generate all the initial makefiles, etc.

echo "Boostrapping StarDict dictionary..."

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

# PKG_NAME="stardict"
REQUIRED_AUTOMAKE_VERSION=1.9

(test -f $srcdir/configure.ac \
  && test -f $srcdir/ChangeLog \
  && test -d $srcdir/src) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level stardict directory"
    exit 1
}

GNOMEDOC=`which yelp-build`
if test -z $GNOMEDOC; then
    echo "*** The tools to build the documentation are not found,"
    echo "    please intall the yelp-tools package ***"
    exit 1
fi

which gnome-autogen.sh || {
    echo "You need to install gnome-common package"
    exit 1
}
# USE_GNOME2_MACROS=1
NOCONFIGURE=yes . gnome-autogen.sh "$@"
