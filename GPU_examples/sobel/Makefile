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
LIBS	:= glut GL GLU freeglut Xxf86vm
 
.PHONY:     clean
 
$(TARGET):  $(OBJ)
	g++ -fPIC -m64  -o $@ $(OBJ) $(foreach D,$(LIB_DIR),-L$D) $(foreach L, $(LIBS), -l$L) $(LDFLAGS)
 
 
clean:
	$(RM) $(TARGET) $(OBJ)

