CC = gcc
LIBS := dbus-1
CFLAGS = $(shell pkg-config --cflags dbus-1)

LIBS_INC := $(foreach lib,$(LIBS),-l$(lib))

SERVICE_FILE_NAME = spotify-listener.service
SERVICE_INSTALL_PATH = /lib/systemd/user

IDIR = ../include
ODIR = ../obj
BIN_DIR = ../bin

_DEPS = utils.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJS = utils.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

_EXE_DEPS = spotify-listener.h spotifyctl.h
EXE_DEPS = $(patsubst %,$(IDIR)/%,$(_EXE_DEPS))

_EXE_OBJS = spotify-listener.o spotifyctl.o
EXE_OBJS = $(patsubst %,$(ODIR)/%,$(_EXE_OBJS))

_EXES = spotify-listener spotifyctl
EXES = $(patsubst %,$(BIN_DIR)/%,$(_EXES))

INSTALL_DIR = /usr/local/bin

debug: CFLAGS += -g


all: spotifyctl spotify-listener

debug: spotifyctl spotify-listener

install: spotifyctl spotify-listener
	mkdir -p $(INSTALL_DIR)
	cp $(EXES) $(INSTALL_DIR)
	cp $(SERVICE_FILE_NAME) $(SERVICE_INSTALL_PATH)
	@echo
	@echo "You can follow the below steps if you are using systemd"
	@echo "To enable spotify-listener at startup run:"
	@echo "systemd --user enable spotify-listener"
	@echo "To start spotify-listener right now, run:"
	@echo "systemd --user start spotify-listener"


spotify-listener: $(OBJS) $(ODIR)/spotify-listener.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/spotify-listener $^ $(CFLAGS) $(LIBS_INC)

spotifyctl: $(OBJS) $(ODIR)/spotifyctl.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/spotifyctl $^ $(CFLAGS) $(LIBS_INC)

$(ODIR)/%.o: %.c $(DEPS) $(EXE_DEPS)
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS_INC)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core vgcore.* $(IDIR)/*~ $(BIN_DIR)/*

