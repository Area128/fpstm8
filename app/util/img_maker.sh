#!/bin/sh

# Input file
FWFILE=$1

# Output file
IMGFILE=$2

# Settings
HDRSECTOR=$3
FWSECTOR=$4
BOOT_ADDR=$5
PARTSIZE_MB=$6
LOOPDEV=/dev/loop99
VOLUMENAME=STM8FWUPDT

# Temp files
HDRFILE=header.bin

le16 () { # little endian 16 bit binary output 1st param: integer to 2nd param: file 
  v=`awk -v n=$1 'BEGIN{printf "%04X", n;}'`
  echo -n -e "\\x${v:2:2}\\x${v:0:2}" >> $2
}

if test -f "$IMGFILE"; then
	rm $IMGFILE
fi

if test -f "$HDRFILE"; then
	rm $HDRFILE
fi

# Create a new image file
dd if=/dev/zero of=./$IMGFILE bs=1M seek=$((PARTSIZE_MB-1)) count=1

# Create partition table
parted -s ./$IMGFILE mklabel msdos

# Create FAT partition
parted ./$IMGFILE mkpart primary fat16 2048s 100%

# Setup loop device
sudo losetup -P $LOOPDEV ./$IMGFILE

# Create FS on new partition
sudo mkfs.fat -n $VOLUMENAME ${LOOPDEV}p1

# Play with new partition here
sudo mcopy -i ${LOOPDEV}p1 ./img_content/* ::

# Detach loop device 
sudo losetup -d $LOOPDEV

# Prepare the header file
## Firmware Size
SIZE=`stat -L -c %s $FWFILE`
echo LEN > ./$HDRFILE
le16 $SIZE ./$HDRFILE

## CRC
CRC=`python3 ./util/crc8_calc.py -i $FWFILE`
echo CRC >> ./$HDRFILE
le16 $CRC ./$HDRFILE

## Destination address
echo DST >> ./$HDRFILE
le16 $(($BOOT_ADDR)) ./$HDRFILE

# Add header and binary to the image
dd conv=notrunc if=./$HDRFILE of=./$IMGFILE bs=512 seek=$HDRSECTOR
dd conv=notrunc if=./$FWFILE of=./$IMGFILE bs=512 seek=$FWSECTOR

# cleanup
rm $HDRFILE

# Check the image
hexdump -C ./$IMGFILE