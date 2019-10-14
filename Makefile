CPPFLAGS += -MD -MP
CFLAGS += -Wall -Wextra -fexceptions -fstack-clash-protection -fstack-protector-strong -Werror=implicit-function-declaration
CFLAGS += -Wfloat-equal -Wlogical-op -Wshift-overflow=2 -Wduplicated-cond -Wcast-qual -Wcast-align
#CFLAGS += -Wconversion
#CFLAGS += -fsanitize=address -fsanitize=undefined
CFLAGS += -Wno-unused-parameter
CFLAGS += -Os -std=c11
CFLAGS += -static
CXXFLAGS += $(CFLAGS)

SRCDIR = src
TSTDIR = tests

CPPFLAGS += -I$(SRCDIR)

vpath %.c $(SRCDIR)
vpath %.c $(TSTDIR)

TARGET = lfs-tool
TEST_TARGET = test

MAIN = main.c

src_search = $(patsubst $(1)/%,%,$(shell find $(1) -name *.c))
dir_search = $(filter-out ./,$(sort $(dir $(1))))

APP_SRC = $(call src_search,$(SRCDIR))
SRC_DIRS = $(call dir_search,$(APP_SRC))

TST_SRC = $(call src_search,$(TSTDIR)) $(filter-out $(MAIN),$(APP_SRC))
TST_DIRS = $(call dir_search,$(TST_SRC))

APP_OBJ = $(APP_SRC:.c=.o)
APP_DEP = $(APP_SRC:.c=.d)

TST_OBJ = $(TST_SRC:.c=.o)
TST_DEP = $(TST_SRC:.c=.d)

OBJ = $(sort $(APP_OBJ) $(TST_OBJ))
DEP = $(sort $(APP_DEP) $(TST_DEP))

$(TARGET): $(APP_OBJ)
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(TEST_TARGET): $(TST_OBJ)
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

-include $(DEP)

$(APP_OBJ): $(APP_SRC) | $(APP_DIRS)

$(TST_OBJ): $(TST_SRC) | $(TST_DIRS)

$(APP_DIRS):
	mkdir -p $@

$(TST_DIRS):
	mkdir -p $@

.PHONY: clean $(TEST_TARGET)
clean:
	$(RM) -r $(DEP) $(TARGET) $(OBJ) $(APP_DIRS) $(TEST_TARGET) $(TST_DIRS)
