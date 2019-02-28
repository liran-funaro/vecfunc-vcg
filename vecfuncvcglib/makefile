# Default compile parameters
dim?=2
value?=float64
color?=yes

# Test parameters
test?=joint_func
repeat?=1
method?=0
chunksize?=8
datapath?=2d_uint32.msgpack
v1?=v1
v2?=v2


OBJ_DIR=obj
BIN_DIR=bin
PERF_DIR=perf
TESTS_DIR=vcg/tests
CC=g++-8

CPP_FILES=$(shell find ./src -not -path '*/.*/*' -type f -name '*.cpp')
HEADER_FILES=$(shell find . -not -path '*/.*/*' -type f \( -iname \*.hpp -o -iname \*.h \))

INCLUDE_FOLDERS=$(shell find . -type d -name 'include' -not -path '*/.*/*')
SRC_FOLDERS=src

CPP_INCLUDE=$(foreach dir,$(INCLUDE_FOLDERS),-I$(dir))
CPP_SOURCE=$(foreach dir,$(SRC_FOLDERS),$(dir)/*.cpp)

CPP_TEST_SOURCE=$(TESTS_DIR)/$(test).cpp

CPP_FLAGS=-std=c++11 -O3 -Wall -Wextra -Werror -pedantic-errors $(CPP_INCLUDE)
ifeq (color, yes)
all::
	CPP_FLAGS += -fdiagnostics-color=always
endif

SHARED_LIB_FLAGS=-c -fPIC
CPP_DEFINE=-D DIM=$(dim) -D VALUE=${value} 
COMPILE=$(CC) $(CPP_FLAGS) $(CPP_DEFINE) $(CPP_SOURCE)
TEST_COMPILE=$(CC) $(CPP_FLAGS) $(CPP_DEFINE) $(CPP_TEST_SOURCE)

RELEASE_FLAGS=-funroll-loops
DEBUG_FLAGS=-g -D DEBUG


NAME=vecfuncvcg_${dim}d_${value}

BIN_FILE=$(BIN_DIR)/$(NAME)
EXEC=$(BIN_FILE).so
TEST_EXEC=$(BIN_FILE)_test_$(test)

PERF_PATH=$(PERF_DIR)/$(NAME)_$(method)_$(chunksize).data


define run_test
    $(eval TMP := $(shell mktemp))
	python $(TESTS_DIR)/read_val.py $(TESTS_DIR)/$(datapath) $(v1) $(v2) $(ressize) > $(TMP)
	$(1) $(testprefix) ./$(TEST_EXEC) $(TMP) $(repeat) $(method) $(chunksize) 
	@rm $(TMP)
endef


all: buildpath $(EXEC)

test: buildpath $(TEST_EXEC)
	$(call run_test)
	
valgrind: buildpath $(TEST_EXEC)
	$(call run_test, valgrind)
	
perf: buildpath $(TEST_EXEC)
	@if [ ! -d "$(PERF_DIR)" ]; then mkdir $(PERF_DIR); fi
	$(call run_test, perf record -o $(PERF_PATH))
	perf report -i $(PERF_PATH)
	

$(EXEC): $(CPP_FILES) $(HEADER_FILES)
	$(COMPILE) $(RELEASE_FLAGS) $(SHARED_LIB_FLAGS)
	@rm -f $(OBJ_DIR)/*.o
	@mv *.o $(OBJ_DIR)/
	g++ -shared -Wl,-soname,$(EXEC) -o $(EXEC)  $(OBJ_DIR)/*.o
	
	
$(TEST_EXEC): $(CPP_TEST_SOURCE) $(CPP_FILES) $(HEADER_FILES)
	$(TEST_COMPILE) $(DEBUG_FLAGS) -o $(TEST_EXEC)

buildpath:
	@if [ ! -d "$(OBJ_DIR)" ]; then mkdir $(OBJ_DIR); fi
	@if [ ! -d "$(BIN_DIR)" ]; then mkdir $(BIN_DIR); fi

clean:
	rm -f *.o
	if [ -d "$(OBJ_DIR)" ]; then rm -R $(OBJ_DIR); fi
	if [ -d "$(PERF_DIR)" ]; then rm -R $(PERF_DIR); fi

clean-all: clean
	if [ -d "$(BIN_DIR)" ]; then rm -R $(BIN_DIR); fi

