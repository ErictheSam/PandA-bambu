#!/bin/bash
abs_script=$(readlink -e $0)
dir_script=$(dirname $abs_script)
#$dir_script/softfloat/all.sh $@
$dir_script/softfloat/xc7z020-1clg484-VVD_10.0_OSF_sdc.sh $@
return_value=$?
if test $return_value != 0; then
   exit $return_value
fi
#$dir_script/libm/all.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/CHStone/all.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/MachSuite/all.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/hls_study/all.sh $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/omp_simd/5SGXEA7N2F45C1_10.0_O2.sh  $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
#$dir_script/omp_simd/xc7vx690t-3ffg1930-VVD_10.0_O2.sh  $@
#return_value=$?
#if test $return_value != 0; then
#   exit $return_value
#fi
exit 0
