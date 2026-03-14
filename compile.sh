#!/bin/sh

cd assignments/PA2;
make clean; make lexer;

cd ../PA3;
make clean; make parser;

cd ../PA4;
make clean; make semant;

cd ../PA5;
make clean; make cgen;
