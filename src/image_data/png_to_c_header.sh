#!/bin/sh
#
#/****  BEGIN - Self compiling script ****\

##
## This script is meant to generate a header file from
## png image data.  The process is to take a png file,
## pass it through gdk-pixbuf-csource and then to
## reformat the generated header by generating c code
## that will include the data array part of the
## cdk-pixbuf-csource header and convert/output a non-
## string version of the same data.
##
## NOTE: This file will be duplicated to generate the
##       c code and the first three lines will be
##       modified to include a starting (c-style)
##       comment block to comment out all the
##       scripting, leaving the c-code at the bottom.
##       This is done to make sure that compilation
##       errors/warnings are tied to the right line
##       number.


##
## Make sure we were given a png file.
##

if [ "$1" = "" ]; then
    echo "Usage: $0 <FILE.PNG>"
    exit 1
fi



##
## Figure out what to name the new header file
##

PNG=$1
NAME=`echo $PNG | sed 's/.png/_pixdata/i'`
HEADER=${NAME}.h
TMP_HEADER="$0.tmp.header.h"

echo "PNG    : $PNG"
echo "NAME   : $NAME"
echo "HEADER : $HEADER"
echo ""



##
## Convert the PNG data to a temporary header
##

echo "Converting '$PNG' to temp header '$TMP_HEADER' ..."
gdk-pixbuf-csource --struct --name=${NAME} $PNG > $TMP_HEADER || exit 1



##
## Generate header to include for compilation
##

INC_HEADER="$0.inc.header.h"

echo "Generating include header '$INC_HEADER' ..."

echo "static char tmp_pixel_data[] = "              >  $INC_HEADER
sed -n '/[ \t]*\".*$/ p' $TMP_HEADER | sed 's/,$//' >> $INC_HEADER
echo ";"                                            >> $INC_HEADER



##
## Generate the code that will include the header above
##

TMP_CODE="$0.code.c"

echo "Generating conversion code '$TMP_CODE' ..."

let L="`wc -l < $0` - 3" # Skip the first three lines!
echo "#include \"$INC_HEADER\"" >   $TMP_CODE
echo ""                         >>  $TMP_CODE
echo "/""**"                    >>  $TMP_CODE
echo ""                         >>  $TMP_CODE
tail -n $L $0                   >>  $TMP_CODE



##
## Compile
##

TMP_BIN="$0.binary"

echo "Compiling conversion app '$TMP_BIN' ..."

# Compile Flags
CFLAGS="-g -Wall"

gcc $CFLAGS -o $TMP_BIN $TMP_CODE

RET="$?" > /dev/null
if [ "$RET" != "0" ]; then
    echo "Compilation error"
    exit 1
fi



##
## Execute code to do conversion.
##

echo "Converting $TMP_HEADER => ${HEADER}_MID ..."

./$TMP_BIN > ${HEADER}_MID



##
## Generate final header
##

echo "Generating final header '$HEADER' ..."

head -n 2 $TMP_HEADER                          >  $HEADER
echo "static guint8 ${NAME}_pixel_data[] = {"  >> $HEADER
cat ${HEADER}_MID                              >> $HEADER
echo ""                                        >> $HEADER
echo "};"                                      >> $HEADER

REGEXP="s/^  \(.*\/\* pixel_data.*\)/  ${NAME}_pixel_data \/\* pixel_data \*\//"

# gdk-pixbuf-csource adds 2 extra empty lines so get rid of one of them.
L=`wc -l < $TMP_HEADER`
let L="$L - `tail -n 2 $TMP_HEADER | grep '^[ \t]*$' | wc -l`"

head -n $L $TMP_HEADER | \
sed -n '/^  \"/ !p' | \
sed -n '/^\// !p' | \
sed -e "$REGEXP"                               >> $HEADER



##
## Cleanup
##

rm -rf $TMP_HEADER $INC_HEADER $TMP_CODE ${HEADER}_MID $TMP_BIN

echo ""
echo "Created '${HEADER}'"

exit

\***** END - Self compiling script *******/

#include <stdio.h>

int main(int argc, char **argv)
{
    char *data = tmp_pixel_data;
    size_t len = sizeof(tmp_pixel_data) -1; // -1 for string terminator '\0'
    size_t i;
    int output = 0;

    printf("  ");
    for ( i = 0; i < len; i++ ) {

        printf("0x%.2x", (0xFF & *data));
        output++;

        if ( i+1 < len ) {
            printf(",");
        }
        if ( output >= 15 ) {
            output = 0;
            printf("\n  ");
        }        

        data++;
    }

    return 0;
}
