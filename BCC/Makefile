SRC = ..\test\*.cpp ..\src\*.cpp 
INCLUDES = ..\test\*.h ..\inc\*.h 
LIBS = ..\lib\*.lib 
INCLUDEPATH = C:\bc31\include;..\test;..\inc;
LIBSPATH = C:\bc31\lib;..\lib;
TARGET = MTHR.exe
$(TARGET):	$(SRC) $(INCLUDES) configuration.xmi
	bcc.exe -mh -v -e$(TARGET) -I$(INCLUDEPATH) -L$(LIBSPATH) $(SRC) $(LIBS)
all:	$(TARGET)
clean:
	-del *.obj
	-del $(TARGET)
	-del *.asm
configuration.xmi:	
	configuration.xmi