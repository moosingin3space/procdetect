CC=clang

all : procdetect

procdetect : procdetect.c proc_tbl.c proc_tbl.h
	${CC} -Wall -Werror -O2 -std=c99 -o procdetect procdetect.c proc_tbl.c
