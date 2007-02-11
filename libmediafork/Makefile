SYSTEM = $(shell uname -s)

ifeq ($(SYSTEM),Linux)
	SYSDEF=-DSYS_LINUX
endif

ifeq ($(SYSTEM),CYGWIN_NT-5.1)
	SYSDEF=-DSYS_CYGWIN
endif

ifeq ($(SYSTEM),FreeBSD)
	CFLAGS += -DSYS_FREEBSD
	LDFLAGS += -pthread -lm
endif
 	
ifeq ($(SYSTEM),NetBSD)
	CFLAGS += -DSYS_NETBSD
	LDFLAGS += -lpthread -lm
endif
 	
ifeq ($(SYSTEM),Linux)
	CFLAGS += -DSYS_LINUX
	LDFLAGS += -lpthread -lm
endif

SRCS = common.c mediafork.c ports.c scan.c work.c decmpeg2.c encavcodec.c \
       update.c demuxmpeg.c fifo.c render.c reader.c muxcommon.c \
       muxmp4.c sync.c decsub.c deca52.c encfaac.c declpcm.c encx264.c \
       decavcodec.c encxvid.c muxavi.c enclame.c muxogm.c encvorbis.c \
       dvd.c  ipodutil.cpp
OTMP = $(SRCS:%.c=%.o) 
OBJS = $(OTMP:%.cpp=%.o)

ifeq ($(SYSTEM),CYGWIN_NT-5.1)
CONTRIBS = ../contrib/lib/liba52.a ../contrib/lib/libavformat.a \
           ../contrib/lib/libavcodec.a ../contrib/lib/libavutil.a \
           ../contrib/lib/libdvdread.a \
           ../contrib/lib/libfaac.a ../contrib/lib/libmp3lame.a \
           ../contrib/lib/libmpeg2.a ../contrib/lib/libmpeg2convert.a \
           ../contrib/lib/libvorbis.a ../contrib/lib/libvorbisenc.a \
           ../contrib/lib/libvorbisfile.a ../contrib/lib/libogg.a \
           ../contrib/lib/libsamplerate.a ../contrib/lib/libx264.a \
           ../contrib/lib/libxvidcore.a  ../contrib/lib/libmp4v2.a
else
CONTRIBS = ../contrib/lib/liba52.a ../contrib/lib/libavformat.a \
           ../contrib/lib/libavcodec.a ../contrib/lib/libavutil.a \
           ../contrib/lib/libdvdread.a ../contrib/lib/libdvdcss.a \
           ../contrib/lib/libfaac.a ../contrib/lib/libmp3lame.a \
           ../contrib/lib/libmpeg2.a ../contrib/lib/libmpeg2convert.a \
           ../contrib/lib/libvorbis.a ../contrib/lib/libvorbisenc.a \
           ../contrib/lib/libvorbisfile.a ../contrib/lib/libogg.a \
           ../contrib/lib/libsamplerate.a ../contrib/lib/libx264.a \
           ../contrib/lib/libxvidcore.a  ../contrib/lib/libmp4v2.a
endif
BUILD = $(shell date "+%Y%m%d")00

CFLAGS += -I../contrib/include -D__LIBMEDIAFORK__ -DUSE_PTHREAD -DHB_VERSION=\"0.8.0b1\" -DHB_BUILD=$(BUILD) $(SYSDEF)

CXXFLAGS += -I../contrib/include -D__LIBMEDIAFORK__ -DUSE_PTHREAD -DHB_VERSION=\"0.8.0b1\" -DHB_BUILD=$(BUILD) $(SYSDEF)

ifeq ($(SYSTEM),CYGWIN_NT-5.1)
all: libmediafork.a libmediafork.dll
else
all: libmediafork.a libmediafork.so
endif

libmediafork.a: $(OBJS)
	@echo "Library $@"
	@ar ru $@ $(OBJS)
	@ranlib $@

libmediafork.so: $(OBJS)
	@echo "Shared library $@"
	@g++ -o $@ $(OBJS) $(CONTRIBS) -shared $(CFLAGS)  || \
	( echo "Compile line for $@ was:"; echo $$CMD; false )

libmediafork.dll: $(OBJS)
	@echo "Shared library $@"
	@g++ -o $@ $(OBJS) $(CONTRIBS) -shared $(CFLAGS)  || \
	( echo "Compile line for $@ was:"; echo $$CMD; false )

%.o: %.c
	@echo "Cc $@"
	@CMD="$(CC) $(CFLAGS) -o $@ -c $<"; $$CMD || \
	  ( echo "Compile line for $@ was:"; echo $$CMD; false )
%.o: %.cpp
	@echo "Cc $@"
	@CMD="$(CC) $(CFLAGS) -o $@ -c $<"; $$CMD || \
	( echo "Compile line for $@ was:"; echo $$CMD; false )

 	
clean:
	@echo "Clean libmediafork.a"
	@$(RM) libmediafork.*
	@echo "Clean libmediafork.so"
	@$(RM) libmediafork.so
	@echo "Clean $(OBJS)"
	@$(RM) $(OBJS)

.depend: $(SRCS)
	@echo "Checking dependencies..."
	@$(RM) .depend
	@$(foreach SRC, $(SRCS), $(CC) -MM $(SRC) $(CFLAGS) >> .depend;)
	
-include .depend

