CC  = g++
CFLAGS    =
TARGET  = main	
SRCS    = main.cpp
OBJS    = $(SRCS:.cpp=.o)
 

INCDIR  = -I../inc 
LIBDIR  = -L../libs
LIBS = -lwsock32 -lws2_32
LIB_PORTAUDIO = -lportaudio_x64

MACROS = -DPORT=49152 -DHEADER_LENGTH=64 -DBUFFER_LENGTH=122880

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBDIR) $(LIBS)
	

$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) $(INCDIR) -c $(SRCS)


all: clean $(OBJS) $(TARGET)

notrigger: 
	$(CC) $(SRCS) $(CFLAGS) $(INCDIR) $(LIBDIR) $(LIB_PORTAUDIO) $(LIBS) -o ../build/scab-notrigger.exe $(MACROS) -DENTRIG=0
lsl:
	$(CC) $(SRCS) $(CFLAGS) $(INCDIR) $(LIBDIR) $(LIB_PORTAUDIO) $(LIBS) -llsl -o ../build/scab-lsl.exe $(MACROS) -DENTRIG=1 -DTRIG_DEV=2
nidaq:
	$(CC) $(SRCS) $(CFLAGS) $(INCDIR) $(LIBDIR) $(LIB_PORTAUDIO) $(LIBS) -lNIDAQmx -o ../build/scab-nidaq.exe $(MACROS) -DENTRIG=1 -DTRIG_DEV=0

clean:
	-rm -f $(OBJS) $(TARGET) *.d