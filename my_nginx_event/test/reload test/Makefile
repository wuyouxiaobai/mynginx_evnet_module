# 编译器
CXX = g++
# 编译选项
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread
# 目标可执行文件名
TARGET = reload_example
# 源文件
SRCS = reload_example.cpp
# 对象文件
OBJS = $(SRCS:.cpp=.o)

# 默认目标
all: $(TARGET)

# 编译目标
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# 生成对象文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	rm -f $(OBJS) $(TARGET)

# 运行程序
run: $(TARGET)
	./$(TARGET)

# 发送 SIGHUP 信号
reload:
	@if [ -f $(TARGET).pid ]; then \
		kill -SIGHUP `cat $(TARGET).pid`; \
		echo "Sent SIGHUP to process `cat $(TARGET).pid`"; \
	else \
		echo "No running process found."; \
	fi

# 后台运行程序
start: $(TARGET)
	./$(TARGET) & echo $$! > $(TARGET).pid
	echo "Started $(TARGET) with PID `cat $(TARGET).pid`"

# 停止程序
stop:
	@if [ -f $(TARGET).pid ]; then \
		kill `cat $(TARGET).pid`; \
		rm -f $(TARGET).pid; \
		echo "Stopped process `cat $(TARGET).pid`"; \
	else \
		echo "No running process found."; \
	fi

# 显示帮助信息
help:
	@echo "Usage: make [target]"
	@echo "Targets:"
	@echo "  all       - Build the program (default)"
	@echo "  clean     - Remove generated files"
	@echo "  run       - Run the program in the foreground"
	@echo "  start     - Run the program in the background"
	@echo "  stop      - Stop the background program"
	@echo "  reload    - Send SIGHUP to the background program"
	@echo "  help      - Show this help message"

.PHONY: all clean run start stop reload help