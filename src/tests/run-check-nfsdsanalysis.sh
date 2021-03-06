#!/bin/sh -x
#
# (c) Copyright 2008, Hewlett-Packard Development Company, LP
#
#  See the file named COPYING for license details
#
# test script

set -e 

SRC=$1

do_check() {
    perl $SRC/check-data/clean-timing.pl < $1.tmp >$1.txt
    cmp $1.txt $SRC/check-data/$1.ref
    rm $1.tmp
}

# Odd order for arguments to preserve ordering in the reference files.
# switching -b, -c would require fixing the reference files.

../analysis/nfs/nfsdsanalysis -a -c 1 -c 2,no_cube_time,no_print_rates -b '' \
    -d 64000000c20f0300200000000127ef590364240c4fdf0058400000009a871600 \
    -d a63c6021c20f0300200000000117721df1e3040064000000400000009a871600 \
    -d e8535201c2cca305200000000079de25139fc708d6390035400000006bfc0800 \
    -e e8535201c2cca3052000000000393dcb82c6fc08d6390035400000006bfc0800 \
    -e $SRC/check-data/nfs.set6.20k.testfhs \
    -f -g -i reply_order -i request_order -i overlapping_reorder -i overlapping_reorder=0.01 -j \
    $SRC/check-data/nfs.set6.20k.ds >check.nfsdsanalysis.tmp
do_check check.nfsdsanalysis

../analysis/nfs/nfsdsanalysis -a -c 1 -c 3,no_cube_host,sql_output -b '' \
    -d 059f13004fb46a0d2000000002725fa8b2c39a0060f8073d059f13004fb46a00 \
    -d 059f13004fb46a0d2000000004de9a890a4af00ebadc0762059f13004fb46a00 \
    -d 059f13004fb46a0d2000000004de9a85074af00ebadc0762059f13004fb46a00 \
    -e 87d874012ece4f0520000000040ba0561d83d10060f8073e87d874012ece4f00 \
    -f -g -i reply_order -i request_order -i overlapping_reorder -i overlapping_reorder=0.01 -j \
    $SRC/check-data/nfs-2.set-0.20k.ds >nfs-2.set-0.20k.tmp
do_check nfs-2.set-0.20k

../analysis/nfs/nfsdsanalysis -a -c 1 -c 4,no_cube_host_detail,sql_output,sql_create_table -b '' \
    -d d5554d0089f4a8052000000000f71819cd080a051aaa0717d5554d0089f4a800 \
    -d b3aace985185f1002000000004521cffd6c2be0afa058e006ceb3200580bf600 \
    -e ee8d3c02707bea0b20000000001603676d27261200fb07e16200000083ecb200 \
    -f -g -i reply_order -i request_order -i overlapping_reorder -i overlapping_reorder=0.01 -j \
    $SRC/check-data/nfs-2.set-1.20k.ds >nfs-2.set-1.20k.tmp 
do_check nfs-2.set-1.20k

if [ `whoami` = anderse -a -f ../analysis/nfs/set-5/cqracks.00000-00049.ds ]; then
    ../analysis/nfs/nfsdsanalysis -i ignore_server,ignore_client -i ignore_server -i ignore_client -i '' ../analysis/nfs/set-5/cqracks.000*ds | perl $SRC/check-data/clean-timing.pl >sequentiality.out
    cmp sequentiality.out ../analysis/nfs/set-5/sequentiality.out
fi

exit 0
