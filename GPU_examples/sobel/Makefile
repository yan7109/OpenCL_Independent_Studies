TARGET  := display
OBJ := display.o parse_ppm.o
 
WARN_FLAGS  := -W -Wall -Wimplicit -Wswitch -Wformat -Wchar-subscripts \
        -Wparentheses -Wmultichar -Wtrigraphs -Wpointer-arith \
        -Wcast-align -Wreturn-type -Wno-unused-function
OPT_FLAGS   := -fno-strict-aliasing -O3
INCLUDES    := -I/$(CUDA_HOME)/include
LIBRARY_PATH    := -L/usr/lib64/
LIB     := -lOpenCL
CXXFLAGS    := -m64 -DUNIX $(WARN_FLAGS) $(OPT_FLAGS) $(INCLUDES)
LDFLAGS     := $(LIBRARY_PATH) $(LIB)
LIB_DIR	:= . 
LIBS	:= X11 GL GLU freeglut Xxf86vm
XTRA_LIB_DIR	:= /usr/lib/gcc/x86_64-linux-gnu/
X11_LIB	:=	X11
 
.PHONY:     clean
 
$(TARGET):  $(OBJ)
	g++ -fPIC -m64  -o $@ $(OBJ) $(LDFLAGS) $(foreach D,$(LIB_DIR),-L$D) $(foreach L, $(LIBS), -l$L) $(foreach D,$(XTRA_LIB_DIR),-L$D) $(foreach L, $(X11_LIB), -l$L) 
 
 
clean:
	$(RM) $(TARGET) $(OBJ)

