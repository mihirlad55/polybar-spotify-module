CC = gcc
LIBS := dbus-1 gio-2.0 glib-2.0
CFLAGS = $(shell pkg-config --cflags --libs dbus-1 gio-2.0 glib-2.0)

LIBS_INC := $(foreach lib,$(LIBS),-l$(lib))

PKG_NAME = polybar-spotify-module
BASE_INSTALL_PREFIX =

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

LICENSE_FILE = ../LICENSE
README_FILE = ../README.md
SERVICE_FILE_NAME = spotify-listener.service

BIN_INSTALL_DIR = /usr/bin
LICENSE_INSTALL_PATH = /usr/share/licenses/$(PKG_NAME)/LICENSE
README_INSTALL_PATH = /usr/share/doc/$(PKG_NAME)/README.md
SERVICE_INSTALL_PATH = /usr/lib/systemd/user/spotify-listener.service

debug: CFLAGS += -g


all: spotifyctl spotify-listener

debug: spotifyctl spotify-listener

install: spotifyctl spotify-listener
	install -Dm755 -t $(BASE_INSTALL_PREFIX)$(BIN_INSTALL_DIR) $(EXES)
	install -Dm644 $(LICENSE_FILE) $(BASE_INSTALL_PREFIX)$(LICENSE_INSTALL_PATH)
	install -Dm644 $(README_FILE) $(BASE_INSTALL_PREFIX)$(README_INSTALL_PATH)
	install -Dm644 $(SERVICE_FILE_NAME) $(BASE_INSTALL_PREFIX)$(SERVICE_INSTALL_PATH)
	@echo
	@echo "You can follow the below steps if you are using systemd"
	@echo "To enable spotify-listener at startup run:"
	@echo "systemd --user enable spotify-listener"
	@echo "To start spotify-listener right now, run:"
	@echo "systemd --user start spotify-listener"

uninstall:
	rm $(addprefix $(BIN_INSTALL_DIR)/,$(_EXES))
	rm $(LICENSE_INSTALL_PATH)
	rm $(README_INSTALL_PATH)
	rm $(SERVICE_INSTALL_PATH)

spotify-listener: $(OBJS) $(ODIR)/spotify-listener.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/spotify-listener $^ $(CFLAGS) $(LIBS_INC)

spotifyctl: $(OBJS) $(ODIR)/spotifyctl.o
	mkdir -p $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/spotifyctl $^ $(CFLAGS) $(LIBS_INC)

$(ODIR)/%.o: %.c $(DEPS) $(EXE_DEPS)
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS_INC)

.PHONY: clean uninstall

clean:
	rm -f $(ODIR)/*.o *~ core vgcore.* $(IDIR)/*~ $(BIN_DIR)/*
