TARGET?=fft
CONFIG?=haswell-loadslice
include ../shared/Makefile.shared

$(TARGET): $(TARGET).o
	$(CC) $(TARGET).o -lm $(SNIPER_LDFLAGS) -o $(TARGET)

run_$(TARGET):
	../../run-sniper -v -n 1 -c $(CONFIG) -- ./fft -p 1

CLEAN_EXTRA=viz
