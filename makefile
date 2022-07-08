BINARY_NAME = vulkan_lib
AUTHOR = Nicholas Warfield

SOURCE_DIR = src
OBJECT_DIR = src/obj
INCLUDE_DIR = include
LIBRARY_DIR = lib
RESOURCE_DIR = resources
BUILD_DIR = build
TEST_DIR = tests

CC = ccache clang++
OPT = O2

CFLAGS = -std=c++17 -Wall -Wextra -Wno-missing-braces -DDEBUG
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

CFLAGS += -I$(INCLUDE_DIR) -I$(LIBRARY_DIR)

HEADERS = $(wildcard *, $(INCLUDE_DIR)/*.hpp)
SOURCE = $(wildcard *, $(SOURCE_DIR)/*.cpp)
OBJECTS := $(patsubst %.cpp,%.o, $(notdir $(SOURCE)))
OBJECTS := $(addprefix $(OBJECT_DIR)/, $(OBJECTS))

TEST_SOURCE = $(wildcard *, $(TEST_DIR)/*.cpp)
TEST_OBJECTS := $(patsubst %.cpp,%.o, $(notdir $(TEST_SOURCE)))
TEST_OBJECTS := $(addprefix $(TEST_DIR)/obj/, $(TEST_OBJECTS))
TEST_OBJECTS += $(OBJECTS)
TEST_OBJECTS := $(filter-out $(OBJECT_DIR)/main.o, $(TEST_OBJECTS))

RUN_ARGS =
ifeq (run,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_ARGS):;@:)
else ifeq (test,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_ARGS):;@:)
else ifeq (bench,$(firstword $(MAKECMDGOALS)))
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(RUN_ARGS):;@:)
endif

$(OBJECT_DIR)/%.o: $(SOURCE_DIR)/%.cpp $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) -$(OPT)

$(BUILD_DIR)/$(BINARY_NAME): init-build $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(CFLAGS) -$(OPT) $(LDFLAGS)

$(TEST_DIR)/obj/benchmark_%.o: $(TEST_DIR)/benchmark_%.cpp $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) -O2

$(TEST_DIR)/obj/test_%.o: $(TEST_DIR)/test_%.cpp $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS) -O0 -g --coverage

$(TEST_DIR)/.test: init-tests $(TEST_OBJECTS)
	$(CC) -o $@ $(TEST_OBJECTS) $(CFLAGS) -O0 -g --coverage $(LDFLAGS)

.PHONY: run clean test coverage bench init-tests init-build all

all: $(BUILD_DIR)/$(BINARY_NAME) $(TEST_DIR)/.test

run: $(BUILD_DIR)/$(BINARY_NAME)
	@bspc rule -a '*' -o state=floating # creates the next window in a float state
	@./$(BUILD_DIR)/$(BINARY_NAME)

clean:
	@[ ! -d $(OBJECT_DIR) ] || trash $(OBJECT_DIR)
	@[ ! -d $(BUILD_DIR) ] || trash $(BUILD_DIR)
	@[ ! -d $(TEST_DIR)/obj ] || trash $(TEST_DIR)/obj
	@[ ! -e $(TEST_DIR)/.test ] || trash $(TEST_DIR)/.test

bench: $(TEST_DIR)/.test
	@unbuffer ./$(TEST_DIR)/.test '[benchmark]' $(RUN_ARGS) --benchmark-no-analysis \
		| tee tests/reports/benchmark.txt

test: $(TEST_DIR)/.test
	@unbuffer ./$(TEST_DIR)/.test ~'[benchmark]' $(RUN_ARGS) \
		| tee tests/reports/report.txt

coverage: $(TEST_DIR)/.test
	@$(MAKE) clean
	@$(MAKE) test
	@llvm-cov gcov -o tests/obj tests/test_*.cpp -bcfp > /dev/null
	@unbuffer gcovr -g -f src -f include \
		--html-details tests/reports/html/coverage.html \
		--json tests/reports/coverage.json \
		--txt | tee -a tests/reports/report.txt

init-tests:
	@[ -d $(TEST_DIR)/obj ] || mkdir -p $(TEST_DIR)/obj
	@[ -d $(TEST_DIR)/reports ] || mkdir -p $(TEST_DIR)/reports
	@[ -d $(TEST_DIR)/reports/html ] || mkdir -p $(TEST_DIR)/reports/html

init-build:
	@[ -d $(OBJECT_DIR) ] || mkdir -p $(OBJECT_DIR)
	@[ -d $(BUILD_DIR)  ] || mkdir -p $(BUILD_DIR)
	@[[ !(! -d $(BUILD_DIR)/$(RESOURCE_DIR) && -d $(RESOURCE_DIR)) ]] \
		|| (ln -s "$(realpath $(RESOURCE_DIR))" $(BUILD_DIR); \
		echo "$(RESOURCE_DIR) linked to $(BUILD_DIR)")

