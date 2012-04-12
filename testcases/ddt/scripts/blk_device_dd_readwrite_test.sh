#! /bin/sh
# 
# Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
#  
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as 
# published by the Free Software Foundation version 2.
# 
# This program is distributed "as is" WITHOUT ANY WARRANTY of any
# kind, whether express or implied; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 

# Perform dd read write test on blk device like mtd, mmc, mount point 
# Input  

source "common.sh"
source "st_log.sh"
source "mtd_common.sh"

SKIP_FORMAT=0
############################# Functions #######################################
usage()
{
cat <<-EOF >&2
        usage: ./${0##*/} [-n DEV_NODE] [-d DEVICE_TYPE] [-f FS_TYPE] [-m MNT_POINT] [-b DD_BUFSIZE] [-c DD_CNT] [-i IO_OPERATION] [-l TEST_LOOP] [-s SKIP_FORMAT]
	-n DEV_NODE	optional param; device node like /dev/mtdblock2; /dev/sda1
        -f FS_TYPE      filesystem type like jffs2, ext2, etc
        -m MNT_POINT	mount point 
        -b DD_BUFSIZE 	dd buffer size for 'bs'
        -c DD_CNT 	dd count for 'count'
        -i IO_OPERATION	IO operation like 'wr', 'rd', default is 'wr'
        -d DEVICE_TYPE  device type like 'nand', 'mmc', 'usb' etc
	-l TEST_LOOP	test loop for r/w. default is 1.
	-s SKIP_FORMAT  skip erase/format part and just do r/w 
        -h Help         print this usage
EOF
exit 0
}

############################### CLI Params ###################################

while getopts  :d:f:m:n:b:c:i:l:sh arg
do case $arg in
        n)      
		# optional param
		DEV_NODE="$OPTARG";;
        d)      DEVICE_TYPE="$OPTARG";;
        f)      FS_TYPE="$OPTARG";;
        m)      MNT_POINT="$OPTARG";;
        b)      DD_BUFSIZE="$OPTARG";;
        c)      DD_CNT="$OPTARG";;
	i) 	IO_OPERATION="$OPTARG";;
	l) 	TEST_LOOP="$OPTARG";;
	s)	SKIP_FORMAT=1;;
        h)      usage;;
        :)      test_print_trc "$0: Must supply an argument to -$OPTARG." >&2
                exit 1
                ;;

        \?)     test_print_trc "Invalid Option -$OPTARG ignored." >&2
                usage
                exit 1
                ;;
esac
done

############################ DEFAULT Params #######################
if [ -z $DEV_NODE ]; then
	DEV_NODE=`get_blk_device_node.sh "$DEVICE_TYPE"` || die "error getting device node for $DEVICE_TYPE: $DEV_NODE"
  test_print_trc "DEV_NODE returned from get_blk_device_node is: $DEV_NODE"
fi
: ${MNT_POINT:=/mnt/partition_$DEVICE_TYPE}
: ${IO_OPERATION:='wr'}
: ${TEST_LOOP:='1'}
test_print_trc "DEV_NODE: $DEV_NODE"
test_print_trc "MNT_POINT: $MNT_POINT"
test_print_trc "FS_TYPE: $FS_TYPE"

############# Do the work ###########################################
#do_cmd "mount" | grep $DEV_NODE 
#if [ $? -ne 0]; then
#	test_print_trc "Erasing/Formatting this partition and then mount it"
#	do_cmd blk_device_erase_format_part.sh -d "$DEVICE_TYPE" -n "$DEV_NODE" -f "$FS_TYPE"
#	do_cmd blk_device_do_mount.sh -n "$DEV_NODE" -f "$FS_TYPE" -d "$DEVICE_TYPE" -m "$MNT_POINT"
#fi
if [ $SKIP_FORMAT -ne 1 ]; then 
	if [ -n "$FS_TYPE" ]; then
		do_cmd blk_device_prepare_format.sh -d "$DEVICE_TYPE" -n "$DEV_NODE" -f "$FS_TYPE" -m "$MNT_POINT"
	else
		do_cmd blk_device_prepare_format.sh -d "$DEVICE_TYPE" -n "$DEV_NODE" -m "$MNT_POINT"
	fi
fi

# find out what is FS in the device
if [ -z "$FS_TYPE" ]; then
  FS_TYPE=`mount | grep $DEV_NODE | cut -d' ' -f5`
  test_print_trc "Current FS_TYPE: ${FS_TYPE}"
fi

test_print_trc "Doing read/write test for $TEST_LOOP times"
x=0
while [ $x -lt $TEST_LOOP ]
do
	do_cmd date	
  TEST_FILE="${MNT_POINT}/test_file_$$"
	case $IO_OPERATION in
		wr)
      #SRC_FILE='/dev/shm/srctest_file'
      SRC_FILE="/home/root/srctest_file_${DEVICE_TYPE}_$$"
      do_cmd "dd if=/dev/urandom of=$SRC_FILE bs=$DD_BUFSIZE count=$DD_CNT"
			do_cmd dd if="$SRC_FILE" of="$TEST_FILE" bs=$DD_BUFSIZE count=$DD_CNT
      do_cmd diff "$SRC_FILE" "$TEST_FILE"
			do_cmd dd if=$TEST_FILE of=/dev/null bs=$DD_BUFSIZE count=$DD_CNT
      do_cmd rm "$SRC_FILE"
		;;
		write_in_bg)
			do_cmd dd if=/dev/urandom of="$TEST_FILE" bs=$DD_BUFSIZE count=$DD_CNT &
		;;
		rd)
			do_cmd dd if=/dev/urandom of="$TEST_FILE" bs=$DD_BUFSIZE count=$DD_CNT
			do_cmd dd if="$TEST_FILE" of=/dev/null bs=$DD_BUFSIZE count=$DD_CNT
		;;
		*)
		test_print_err "Invalid IO operation type in $0 script"
		exit 1;
		;;	
	esac
	do_cmd rm "$TEST_FILE"
	x=$((x+1))
	do_cmd date
done
[ $SKIP_FORMAT -eq 1 ] || do_cmd blk_device_unprepare.sh -n "$DEV_NODE" -d "$DEVICE_TYPE" -f "$FS_TYPE"



