#!/bin/sh

rm -rf sendTest sendTest.dSYM master master.dSYM

g++ -Wall -ansi -g master.cpp RS485.cpp util.c wiringSerial.c -o master
g++ -Wall -ansi -g sendtest.cpp RS485.cpp util.c wiringSerial.c -o sendTest
g++ -Wall -ansi -g benchmark.cpp RS485.cpp util.c wiringSerial.c -o benchmark
