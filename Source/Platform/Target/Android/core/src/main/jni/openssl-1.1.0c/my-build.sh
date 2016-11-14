#export MACHINE=arm
#export SYSTEM=android
#export TARGETMACH=arm-linux-androideabi
#export BUILDMACH=i686-pc-linux-gnu
#export CROSS=arm-linux-androideabi
#export CROSS_COMPILE=${CROSS}-
#export CC=gcc
#export LD=ld
#export AS=as
#export AR=ar
setarch i386 ./config -m32 shared no-asm no-ssl3 no-comp no-hw no-engine

