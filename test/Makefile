SYSTEM = $(shell uname -s)

LIBS = a52 mkv avformat avcodec avutil dca dvdread faac mp3lame mpeg2 vorbis \
	   vorbisenc ogg samplerate x264 xvidcore mp4v2 swscale

all: ../HandBrakeCLI

CFLAGS += -I../libhb
CXXFLAGS += -I../libhb
LIBS2 = ../libhb/libhb.a $(LIBS:%=../contrib/lib/lib%.a)
LDFLAGS += $(LIBS2) -ldl

../HandBrakeCLI: test.c parsecsv.c $(LIBS2)
	@CMD="$(CC) $(CFLAGS) -o test.o -c test.c"; $$CMD || \
	( echo "Compile line for $@ was:"; echo $$CMD; false )
	@CMD="$(CC) $(CFLAGS) -o parsecsv.o -c parsecsv.c"; $$CMD || \
	( echo "Compile line for $@ was:"; echo $$CMD; false )
	@echo "Link HandBrakeCLI"
	@CMD="g++ $(CXXFLAGS) -o ../HandBrakeCLI test.o parsecsv.o $(LDFLAGS) -lz -lpthread"; $$CMD || \
	( echo "Compile line for $@ was:"; echo $$CMD; false )
        @CMD="rm -rf ../plugins ; mkdir ../plugins ; cp ../contrib/lib/libquicktime/* ../plugins"; $$CMD


clean:
	@echo "Clean HandBrakeCLI"
	@$(RM) ../HandBrakeCLI 
	@$(RM) ../HandBrakeCLI.exe
	@$(RM) *.o

