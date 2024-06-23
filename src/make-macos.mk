CC     = g++-12
CFLAGS = -Wall
TARGET = main	
SRCS   = main.cpp
OBJS   = $(SRCS:.cpp=.o)

INCDIR  = -I../inc
LIBDIR  = -L../libs/arm64macos
LIBS = -lportaudio -licom

# MACROS = -DPORT=49152 -DHEADER_LENGTH=64 -DBUFFER_LENGTH=122880
MACROS = 

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBDIR) $(LIBS)
	

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) $(INCDIR) -c $(SRCS)


all: clean $(OBJS) $(TARGET)
	
notrigger: 
	$(CC) $(SRCS) $(CFLAGS) $(INCDIR) $(LIBDIR) $(LIBS) -o ../build/macos/scab-notrigger $(MACROS) -DENTRIG=0

lsl:
	$(CC) $(SRCS) $(CFLAGS) $(INCDIR) $(LIBDIR) $(LIBS) -llsl -o ../build/macos/scab-lsl $(MACROS) -DENTRIG=1 -DTRIG_DEV=2

sock:
	$(CC) test-socket.cpp -o ./socket

padevs:
	$(CC) pa_devs.c $(INCDIR) -L../libs/arm64macos -lportaudio -o ../build/macos/padevs

pasine:
	$(CC) paex_sine.c $(INCDIR) -L../libs/arm64macos -lportaudio -o ../build/macos/pasine

clean:
	-rm -f $(OBJS) $(TARGET) *.d