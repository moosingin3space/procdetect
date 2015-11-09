CC=clang

all : procdetect

procdetect : procdetect.c proc_tbl.c proc_tbl.h
	${CC} -Wall -Werror -O2 -o procdetect procdetect.c proc_tbl.c
