# Makefile for Pikifen. This should be run in the project's root folder. Check the included manual for more info.

PROG         := pikifen
CC           := g++
SRCS         := $(shell find Source/source -name '*.cpp')
OBJS         := $(SRCS:.cpp=.o)
DEPS         := $(OBJS:.o=.d)
CXXFLAGS     := -std=c++0x -rdynamic -D_GLIBCXX_USE_CXX11_ABI=0 -MMD
LDFLAGS      += -lm `pkg-config --libs allegro-5 allegro_main-5 allegro_audio-5 allegro_image-5 allegro_font-5 allegro_acodec-5 allegro_primitives-5 allegro_dialog-5`
DEBUGFLAGS   := -g -ggdb -Wall -Wno-unknown-pragmas -O0
RELEASEFLAGS := -Wall -Wextra -Wno-unknown-pragmas -O2

.PHONY: all
all: release

.PHONY: debug
debug: CXXFLAGS += $(DEBUGFLAGS)
debug: $(PROG)

.PHONY: release
release: CXXFLAGS += $(RELEASEFLAGS)
release: $(PROG)

$(PROG): $(OBJS)
	$(CC) $^ $(CXXFLAGS) $(LDFLAGS) -o $(PROG)
# If the above does not work and gives linker errors, try the following line instead.
#	$(LINK.cc) $(OBJS) -o $(PROG)

-include $(DEPS)

.PHONY: clean
clean:
	$(RM) $(OBJS)
	$(RM) $(DEPS)
	$(RM) $(PROG)
