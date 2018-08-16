DEF ?= def
DEFGHI = defghi
SET_BENCH = set_bench
PRIORITY_BENCH = priority_bench

OPTLEVEL = -O3

DEFFLAGS = $(OPTLEVEL) --ftransactions=hardware
DEFLIBS = -lpthread -lm -lcpuinfo

CC = clang
CFLAGS = $(OPTLEVEL) -mrtm

DEF_SETS = \
	fhsl_lf.def \
	bt_lf.def \
	mm_ht.def \
	so_ht.def

DEF_PQUEUES = \
	sl_pq.def \
	spray_pq.def

C_SETS = \
	c_fhsl_lf.c \
	c_bt_lf.c \
	c_mm_ht.c \
	c_so_ht.c

C_PQUEUES = \
	c_sl_pq.c \
	c_spray_pq.c

DEFIFILES = $(DEF_SETS:.def=.defi) $(DEF_PQUEUES:.def=.defi)

SET_SRC = $(DEF_SETS) $(C_SETS) utils.c thread_pinner.c set_bench.def
SET_DEF_OBJ = $(SET_SRC:.def=.o)
SET_OBJ = $(SET_DEF_OBJ:.c=.o)

PQUEUE_SRC = $(DEF_PQUEUES) $(C_PQUEUES) utils.c thread_pinner.c priority_bench.def
PQUEUE_DEF_OBJ = $(PQUEUE_SRC:.def=.o)
PQUEUE_OBJ = $(PQUEUE_DEF_OBJ:.c=.o)

all: $(SET_BENCH) $(PRIORITY_BENCH)

$(BENCH): $(BENCH_OBJ)
	$(DEF) -o $@ $(DEFFLAGS) $(DEFLIBS) $^

$(SET_BENCH): $(SET_OBJ)
	$(DEF) -o $@ $(DEFFLAGS) $(DEFLIBS) $^

$(PRIORITY_BENCH): $(PQUEUE_OBJ)
	$(DEF) -o $@ $(DEFFLAGS) $(DEFLIBS) $^

clean:
	rm -f $(SET_BENCH) $(PRIORITY_BENCH) *.defi *.o

set_bench.o: $(DEFIFILES)

priority_bench.o: $(DEFIFILES)

%.ll: %.def
	$(DEF) -o $@ $(DEFFLAGS) -S -emit-llvm $<

%.ll: %.c
	$(CC) -o $@ $(CFLAGS) -S -emit-llvm $<

%.o: %.ll
	$(DEF) -o $@ $(DEFFLAGS) -c $<

%.defi: %.def
	$(DEFGHI) $<
