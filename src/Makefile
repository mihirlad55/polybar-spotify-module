CC = gcc
LIBS := dbus-1
CFLAGS = $(shell pkg-config --cflags dbus-1)

LIBS_INC := $(foreach lib,$(LIBS),-l$(lib))

IDIR = ../include
ODIR = ../obj
BIN_DIR = ../bin

_DEPS = utils.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJS = utils.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

_EXE_DEPS = spotify-listener.h spotify-status.h
EXE_DEPS = $(patsubst %,$(IDIR)/%,$(_EXE_DEPS))

_EXE_OBJS = spotify-listener.o spotify-status.o
EXE_OBJS = $(patsubst %,$(ODIR)/%,$(_EXE_OBJS))

_EXES = spotify-listener spotify-status
EXES = $(patsubst %,$(BIN_DIR)/%,$(_EXES))

INSTALL_DIR = /usr/local/bin

debug: CFLAGS += -g


all: spotify-status spotify-listener

debug: spotify-status spotify-listener

install: spotify-status spotify-listener
	mkdir -p $(INSTALL_DIR)
	cp $(EXES) $(INSTALL_DIR)

spotify-listener: $(OBJS) $(ODIR)/spotify-listener.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/spotify-listener $^ $(CFLAGS) $(LIBS_INC)

spotify-status: $(OBJS) $(ODIR)/spotify-status.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/spotify-status $^ $(CFLAGS) $(LIBS_INC)

$(ODIR)/%.o: %.c $(DEPS) $(EXE_DEPS)
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS_INC)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core vgcore.* $(IDIR)/*~ $(BIN_DIR)/*

