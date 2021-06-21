CPPFLAGS += -MD -MP
CPPFLAGS += -D_XOPEN_SOURCE=700
CPPFLAGS += -D_FILE_OFFSET_BITS=64
CFLAGS += -Wall -Wextra -fexceptions -fstack-protector-strong -Werror=implicit-function-declaration
CFLAGS += -Wfloat-equal -Wlogical-op -Wshift-overflow=2 -Wduplicated-cond -Wcast-qual -Wcast-align
#CFLAGS += -Wconversion -fstack-clash-protection 
#CFLAGS += -fsanitize=address -fsanitize=undefined
CFLAGS += -Wno-unused-parameter
CFLAGS += -Os -std=c11

ifndef NOSTATIC
CFLAGS += -static
endif

CXXFLAGS += $(CFLAGS)


SRCDIR = src
TSTDIR = tests
BUILD_DIR = build

CPPFLAGS += -I$(SRCDIR) -I$(SRCDIR)/mbedtls/include

vpath %.c $(SRCDIR)
vpath %.c $(TSTDIR)

TARGET = lfs-tool
TEST_TARGET = test

MAIN = main.c

src_search = $(patsubst $(1)/%,%,$(shell find $(1) -name *.c -not -path "*/mbedtls/programs/*"))
dir_search = $(filter-out ./,$(sort $(dir $(1))))

APP_SRC = $(call src_search,$(SRCDIR))
# APP_DIRS = $(call dir_search,$(APP_SRC))

TST_SRC = $(call src_search,$(TSTDIR)) $(filter-out $(MAIN),$(APP_SRC))
TST_DIRS = $(addprefix $(BUILD_DIR)/,$(call dir_search,$(TST_SRC)))

APP_OBJ = $(addprefix $(BUILD_DIR)/,$(APP_SRC:.c=.o))
APP_DEP = $(addprefix $(BUILD_DIR)/,$(APP_SRC:.c=.d))

TST_OBJ = $(addprefix $(BUILD_DIR)/,$(TST_SRC:.c=.o))
TST_DEP = $(addprefix $(BUILD_DIR)/,$(TST_SRC:.c=.d))

OBJ = $(sort $(APP_OBJ) $(TST_OBJ))
DEP = $(sort $(APP_DEP) $(TST_DEP))

$(info $(APP_OBJ))
$(info $(DEP))

$(TARGET): $(APP_OBJ)
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(TEST_TARGET): $(TST_OBJ)
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

-include $(DEP)

$(APP_OBJ): | $(APP_DIRS)

$(TST_OBJ): | $(TST_DIRS)

$(APP_DIRS):
	mkdir -p $@

$(TST_DIRS):
	mkdir -p $@

$(BUILD_DIR):
	mkdir -p $@

.PHONY: all clean $(TEST_TARGET)
clean:
	$(RM) -r $(DEP) $(TARGET) $(OBJ) $(APP_DIRS) $(TEST_TARGET) $(TST_DIRS) $(BUILD_DIR)
all: $(TARGET)
