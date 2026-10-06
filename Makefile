CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -O2
#LDFLAGS  := -Wl,--subsystem,console
INCLUDES := -Iinclude
SRCDIR   := src
OBJDIR   := build
OUTDIR   := output
TARGET   := $(OUTDIR)/rtos_simulator.exe

SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS) | $(OUTDIR)
	g++ -std=c++17 -Wall -Wextra -pedantic -O2 build/EventFlags.o build/ExecutionLogger.o build/Kernel.o build/MessageQueue.o build/Mutex.o build/ResourceManager.o build/Scheduler.o build/Semaphore.o build/SimulatorController.o build/StatisticsAnalyzer.o build/Task.o build/TaskManager.o build/TestRunner.o build/TimerManager.o build/WatchdogTimer.o build/main.o -mconsole -o output/rtos_simulator.exe

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OUTDIR):
	mkdir -p $(OUTDIR)

clean:
	rm -rf $(OBJDIR) $(OUTDIR)

run: $(TARGET)
	./$(TARGET)