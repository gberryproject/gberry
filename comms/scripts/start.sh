#!/bin/bash
THIS_DIR=`cd $(dirname $0) && pwd`
CONSOLE_ROOT=`cd $THIS_DIR/.. && pwd`

QTDIR=`cd $THIS_DIR/../lib/Qt/5.4.0 && pwd`

export LD_LIBRARY_PATH=$CONSOLE_ROOT/lib:$QTDIR/lib
export QML2_IMPORT_PATH=$CONSOLE_ROOT/lib/qml

QTCONF=$THIS_DIR/qt.conf
if [ ! -e $QTCONF ]; then
    echo "[Paths]" > $QTCONF
    echo "Prefix = $QTDIR" >> $QTCONF
fi

if [ -z "$1" ]; then
    $THIS_DIR/comms
else
    $THIS_DIR/$1
fi
