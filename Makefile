# # GNU make手册：http://www.gnu.org/software/make/manual/make.html
# # ************ 遇到不明白的地方请google以及阅读手册 *************

MY_ENV ?= other
# 编译器设定和编译选项
CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -std=c99

ifeq (${MY_ENV}, other)
# # 编译目标：src目录下的所有.c文件
CFILES = $(shell find ./ -name "*.c")
OBJS = $(CFILES:.c=.o)
LFILE = $(shell find ./ -name "*.l")
YFILE = $(shell find ./ -name "*.y")
LFC = $(shell find ./ -name "*.l" | sed s/[^/]*\\.l/lex.yy.c/)
YFC = $(shell find ./ -name "*.y" | sed s/[^/]*\\.y/syntax.tab.c/)
LFO = $(LFC:.c=.o)
YFO = $(YFC:.c=.o)

parser: syntax $(filter-out $(LFO),$(OBJS))
	$(CC) -o parser $(filter-out $(LFO),$(OBJS)) -lfl -ly

syntax: lexical syntax-c
	$(CC) -c $(YFC) -o $(YFO)

lexical: $(LFILE)
	$(FLEX) -o $(LFC) $(LFILE)

syntax-c: $(YFILE)
	$(BISON) -o $(YFC) -d -v $(YFILE)

-include $(patsubst %.o, %.d, $(OBJS))

# # 定义的一些伪目标
.PHONY: clean test
test:
	./parser ../Test/test1.cmm
clean:
	rm -f parser lex.yy.c syntax.tab.c syntax.tab.h syntax.output
	rm -f $(OBJS) $(OBJS:.o=.d)
	rm -f $(LFC) $(YFC) $(YFC:.c=.h)
	rm -f *~

# local environment, for development
else
# 编译目标：src目录下的所有.c文件
CFILES = $(shell find ./ -name "*.c")
OBJS = $(patsubst %.c,build/%.o,$(CFILES))
LFILE = $(shell find ./ -name "*.l")
YFILE = $(shell find ./ -name "*.y")
LFC = $(patsubst %.l,build/lex.yy.c,$(LFILE))
YFC = $(patsubst %.y,build/syntax.tab.c,$(YFILE))
LFO = $(patsubst %.c,%.o,$(LFC))
YFO = $(patsubst %.c,%.o,$(YFC))

# Output directory
BUILD_DIR = build

# Ensure build directory exists
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

parser: $(BUILD_DIR) syntax main.c
	$(CC) -o $(BUILD_DIR)/parser main.c $(BUILD_DIR)/syntax.tab.c -lfl -ly

parser_debug: $(BUILD_DIR) syntax main.c
	$(CC) -DDEBUG -o $(BUILD_DIR)/parser main.c $(BUILD_DIR)/syntax.tab.c -lfl -ly

syntax: $(BUILD_DIR) lexical syntax-c
	$(CC) -c $(YFC) -o $(YFO)

lexical: $(BUILD_DIR) $(LFILE)
	$(FLEX) -o $(LFC) $(LFILE)

scanner: $(BUILD_DIR) $(LFC) main.c
	$(CC) main.c $(LFC) -lfl -o $(BUILD_DIR)/scanner

syntax-c: $(BUILD_DIR) $(YFILE)
	$(BISON) -o $(YFC) -d $(YFILE)

syntax-output: $(BUILD_DIR) $(YFILE)
	$(BISON) -o $(YFC) -d -v $(YFILE)

-include $(patsubst %.o, %.d, $(OBJS))

# 定义的一些伪目标
.PHONY: clean test
test: $(BUILD_DIR)/parser
	./$(BUILD_DIR)/parser ../test/test1.cmm
clean:
	rm -rf $(BUILD_DIR)
	rm -f *~

endif