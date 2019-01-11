all: simtbs

CFLAGS = -g -Wall -DDEBUG

#task.o conf.o mem.o power.o report.o dfdm.o dvs.o dmem.o fast.o output.o
simtbs: simtbs.o conf.o kernel.o sm.o report.o policy_bfs.o policy_dfs.o
	gcc -o $@ $^ -lm

simtbs.o: simtbs.h
conf.o: simtbs.h
kernel.o: simtbs.h
sm.o: simtbs.h
report.o: simtbs.h

policy_bfs.o: simtbs.h
policy_dfs.o: simtbs.h

tarball:
	tar czvf simtbs.tgz *.[ch] *.tmpl Makefile
