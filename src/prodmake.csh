#!/bin/csh 
echo Moving into the production directory.
sleep 1
cd ~/mud/prod/
echo Removing old compile output file. 
sleep 1 
rm -f file.mak 
echo Done! 
echo -n 
echo Cleaning old object files for a new COMPLETE compilation. 
sleep 2 
make clean 
echo Done! 
echo -n 
echo Beginning new compilation! 
sleep 1 
time make -j 5 > & file.mak & 
sleep 1 
echo Process Started! 
echo -n 
echo Attaching to file output. Press control-c when compilation is complete to exit. 
tail -f file.mak
