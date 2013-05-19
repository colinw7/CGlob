CC = g++
AR = ar
RM = rm

CDEBUG = -g
LDEBUG = -g

INC_DIR = ../include
OBJ_DIR = ../obj
LIB_DIR = ../lib

all: $(LIB_DIR)/libCGlob.a

SRC = \
CGlob.cpp

OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC))

CPPFLAGS = -I$(INC_DIR) -I.

clean:
	$(RM) -f $(OBJ_DIR)/*.o
	$(RM) -f $(LIB_DIR)/libCGlob.a

$(OBJS): $(OBJ_DIR)/%.o: %.cpp
	$(CC) -c $< -o $(OBJ_DIR)/$*.o $(CPPFLAGS)

.SUFFIXES: .cpp

$(LIB_DIR)/libCGlob.a: $(OBJS)
	$(AR) crv $(LIB_DIR)/libCGlob.a $(OBJS)
