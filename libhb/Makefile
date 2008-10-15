include ../Makefile.config

SYSTEM = $(shell uname -s)

ifeq ($(findstring CYGWIN_NT,$(SYSTEM)),CYGWIN_NT)
	SYSDEF=-DSYS_CYGWIN
endif

ifeq ($(SYSTEM),FreeBSD)
	SYSDEF = -DSYS_FREEBSD
	LDFLAGS += -pthread -lm
endif
 	
ifeq ($(SYSTEM),NetBSD)
	SYSDEF = -DSYS_NETBSD
	LDFLAGS += -lpthread -lm
endif
 	
ifeq ($(SYSTEM),Linux)
	SYSDEF = -DSYS_LINUX $(HB_DEFPPC) -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
	LDFLAGS += -lpthread -lm -ldl
endif

SRCS = common.c hb.c ports.c scan.c work.c decmpeg2.c encavcodec.c enctheora.c \
	   update.c demuxmpeg.c fifo.c render.c reader.c muxcommon.c stream.c \
	   muxmp4.c sync.c decsub.c deca52.c decdca.c encfaac.c declpcm.c encx264.c \
	   decavcodec.c encxvid.c muxmkv.c muxavi.c enclame.c muxogm.c encvorbis.c \
	   dvd.c deblock.c deinterlace.c denoise.c detelecine.c decomb.c lang.c
OTMP = $(SRCS:%.c=%.o) 
OBJS = $(OTMP:%.cpp=%.o)

CONTRIBS = ../contrib/lib/liba52.a ../contrib/lib/libavformat.a \
		   ../contrib/lib/libavcodec.a ../contrib/lib/libavutil.a \
		   ../contrib/lib/libdca.a ../contrib/lib/libdvdread.a  \
		   ../contrib/lib/libfaac.a ../contrib/lib/libmp3lame.a \
		   ../contrib/lib/libmpeg2.a ../contrib/lib/libmpeg2convert.a \
		   ../contrib/lib/libvorbis.a ../contrib/lib/libvorbisenc.a \
		   ../contrib/lib/libvorbisfile.a ../contrib/lib/libogg.a \
		   ../contrib/lib/libsamplerate.a ../contrib/lib/libx264.a \
		   ../contrib/lib/libxvidcore.a  ../contrib/lib/libmp4v2.a \
		   ../contrib/lib/libmkv.a ../contrib/lib/libswscale.a \
		   ../contrib/lib/libtheora.a ../contrib/lib/libfaad.a
		   
ifeq ($(findstring CYGWIN_NT,$(SYSTEM)),CYGWIN_NT)
	CONTRIBS += ../contrib/lib/libz.a ../contrib/lib/libbz2.a
endif

CFLAGS += -I../contrib/include -D__LIBHB__ -DUSE_PTHREAD $(SYSDEF) -Wall -g -O3 -funroll-loops

CXXFLAGS += -I../contrib/include -D__LIBHB__ -DUSE_PTHREAD $(SYSDEF)

ifeq ($(findstring CYGWIN_NT,$(SYSTEM)),CYGWIN_NT)
all: libhb.a libhb.dll
else
all: libhb.a

endif

libhb.a: $(OBJS)
	@echo "Library $@"
	@ar ru $@ $(OBJS)
	@ranlib $@

libhb.so: $(OBJS)
	@echo "Shared library $@"
	@g++ -o $@ $(OBJS) $(CONTRIBS) -shared $(CFLAGS)  || \
	( echo "Compile line for $@ was:"; echo $$CMD; false )

libhb.dll: $(OBJS)
	@echo "Shared library $@"
	@g++ -o $@ $(OBJS) $(CONTRIBS) -shared $(CFLAGS)  || \
	( echo "Compile line for $@ was:"; echo $$CMD; false )

%.o: %.c
	@echo "Cc $@"
	echo $(CC) $(CFLAGS)
	@CMD="$(CC) $(CFLAGS) -o $@ -c $<"; $$CMD || \
	  ( echo "Compile line for $@ was:"; echo $$CMD; false )

%.o: %.cpp
	@echo "Cc $@"
	@CMD="$(CC) $(CFLAGS) -o $@ -c $<"; $$CMD || \
	( echo "Compile line for $@ was:"; echo $$CMD; false )

 	
clean:
	@echo "Clean libhb.a"
	@$(RM) libhb.*
	@echo "Clean libhb.so"
	@$(RM) libhb.so
	@echo "Clean $(OBJS)"
	@$(RM) $(OBJS)

.depend: $(SRCS)
	@echo "Checking dependencies..."
	@$(RM) .depend
	@$(foreach SRC, $(SRCS), $(CC) -MM $(SRC) $(CFLAGS) >> .depend;)
	
-include .depend

