DEF ?= def
DEFGHI = defghi
BENCH = bench

OPTLEVEL = -O3

DEFFLAGS = $(OPTLEVEL) --ftransactions=hardware
DEFLIBS = -lpthread -lm

CC = clang
CFLAGS = $(OPTLEVEL) -mrtm

DEF_STRUCTURES = \
	fhsl_lf.def \
	bt_lf.def \
	sl_pq.def \
	spray_pq.def

C_STRUCTURES = \
	c_fhsl_lf.c \
	c_bt_lf.c \
	c_sl_pq.c \
	c_spray_pq.c \
	c_mm_ht.c \
	c_so_ht.c

DEFIFILES = $(DEF_STRUCTURES:.def=.defi)

BENCH_SRC = $(DEF_STRUCTURES) $(C_STRUCTURES) bench.def
BENCH_DEF_OBJ = $(BENCH_SRC:.def=.o)
BENCH_OBJ = $(BENCH_DEF_OBJ:.c=.o)

all: $(BENCH)

$(BENCH): $(BENCH_OBJ)
	$(DEF) -o $@ $(DEFFLAGS) $(DEFLIBS) $^

clean:
	rm -f $(BENCH) *.defi *.o

bench.o: $(DEFIFILES)

%.o: %.def
	$(DEF) -o $@ $(DEFFLAGS) -c $<

%.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

%.defi: %.def
	$(DEFGHI) $<
