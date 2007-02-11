SYSTEM = $(shell uname -s)

ifeq ($(SYSTEM),CYGWIN_NT-5.1)
	LIBS = a52 avformat avcodec avutil dvdread faac mp3lame mpeg2 vorbis vorbisenc ogg samplerate x264 xvidcore mp4v2
else
        LIBS = a52 avformat avcodec avutil dvdread dvdcss faac mp3lame mpeg2 vorbis vorbisenc ogg samplerate x264 xvidcore mp4v2
endif

all: ../MediaForkCLI

CFLAGS += -I../libmediafork
CXXFLAGS += -I../libmediafork
LIBS2 = ../libmediafork/libmediafork.a $(LIBS:%=../contrib/lib/lib%.a)
LDFLAGS += $(LIBS2)

../MediaForkCLI: test.c $(LIBS2)
	@CMD="$(CC) $(CFLAGS) -o test.o -c test.c"; $$CMD || \
	( echo "Compile line for $@ was:"; echo $$CMD; false )
	@echo "Link MediaForkCLI"
	@CMD="g++ $(CXXFLAGS) -o ../MediaForkCLI test.o $(LDFLAGS) -lz -lpthread"; $$CMD || \
	( echo "Compile line for $@ was:"; echo $$CMD; false )


clean:
	@echo "Clean MediaForkCLI"
	@$(RM) ../MediaForkCLI 
	@$(RM) ../MediaForkCLI.exe
	@$(RM) *.o

