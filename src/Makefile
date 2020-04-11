CC = gcc
LIBS := dbus-1
CFLAGS = $(shell pkg-config --cflags dbus-1)

LIBS_INC := $(foreach lib,$(LIBS),-l$(lib))

IDIR = ../include
ODIR = obj
BIN_DIR = bin

_DEPS = spotify-listener.h utils.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJS = spotify-listener.o utils.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))


spotify-listener: $(OBJS)
	mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/spotify-listener $^ $(CFLAGS) $(LIBS_INC)

$(ODIR)/%.o: %.c $(DEPS)
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS_INC)

debug: $(OBJS)
	mkdir -p $(BIN_DIR)
	$(CC) -g -o $(BIN_DIR)/spotify-listener $^ $(CFLAGS) $(LIBS_INC)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core vgcore.* $(IDIR)/*~

