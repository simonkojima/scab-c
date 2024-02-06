# (1)コンパイラ
CC  = g++
# (2)コンパイルオプション
CFLAGS    =
# (3)実行ファイル名
TARGET  = main	
# (4)コンパイル対象のソースコード
SRCS    = main.cpp
# (5)オブジェクトファイル名
OBJS    = $(SRCS:.cpp=.o)
 
# (6)インクルードファイルのあるディレクトリパス
INCDIR  = -I../inc
 
# (7)ライブラリファイルのあるディレクトリパス
LIBDIR  = -L../libs
 
# (8)追加するライブラリファイル
LIBS = -lwsock32 -lws2_32

LIB_PORTAUDIO = -lportaudio_x64

# (9)ターゲットファイル生成
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBDIR) $(LIBS)
	
# (10)オブジェクトファイル生成
$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) $(INCDIR) -c $(SRCS)

# (11)"make all"で make cleanとmakeを同時に実施。
all: clean $(OBJS) $(TARGET)

notrigger: 
	$(CC) $(SRCS) $(CFLAGS) $(INCDIR) $(LIBDIR) $(LIB_PORTAUDIO) $(LIBS) -o ../build/scab_notrigger.exe -DENTRIG=0
lsl:
	$(CC) $(SRCS) $(CFLAGS) $(INCDIR) $(LIBDIR) $(LIB_PORTAUDIO) $(LIBS) -llsl -o ../build/scab_lsl.exe -DENTRIG=1 -DTRIG_DEV=2
nidaq:
	$(CC) $(SRCS) $(CFLAGS) $(INCDIR) $(LIBDIR) $(LIB_PORTAUDIO) $(LIBS) -lNIDAQmx -o ../build/scab_nidaq.exe -DENTRIG=1 -DTRIG_DEV=0
# (12).oファイル、実行ファイル、.dファイルを削除
clean:
	-rm -f $(OBJS) $(TARGET) *.d