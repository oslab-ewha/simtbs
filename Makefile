all: simtbs

CFLAGS = -g -Wall -DDEBUG

POLICY = rr rrf bfa dfa binary

SIMTBS_OBJS = simtbs.o conf.o kernel.o sm.o mem.o report.o wl.o
POLICY_OBJS = $(POLICY:%=policy_%.o)

simtbs: $(SIMTBS_OBJS) $(POLICY_OBJS)
	gcc -o $@ $^ -lm

$(SIMTBS_OBJS) $(POLICY_OBJS): simtbs.h

$(POLICY_OBJS): simtbs.h

tarball:
	tar czvf simtbs.tgz *.[ch] *.tmpl Makefile

clean:
	rm -f simtbs *.o
