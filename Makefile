CXX       = g++
CFLAGS    = -Wall
CPPFLAGS  = $(CFLAGS) -I/usr/local/include -L/usr/local/lib -Iinclude/ -I/include

LIBRARIES = SimpleAmqpClient
LIBS      = -pthread $(addprefix -l,$(LIBRARIES))

COMMON_CPPS = lb_error.cpp lb_functions.cpp

LEADERBOARD_SOURCES = leaderboard.cpp $(COMMON_CPPS)
LEADERBOARD_TARGET  = $(LEADERBOARD_SOURCES:.cpp=.o)

PRODUCE_ONE_SOURCES = produce_one.cpp
PRODUCE_ONE_TARGET  = $(LEADERBOARD_SOURCES:.cpp=.o)

MONITOR_SOURCES = monitor.cpp $(COMMON_CPPS)
MONITOR_TARGET  = $(MONITOR_SOURCES:.cpp=.o)

LOAD_SOURCES = load.cpp $(COMMON_CPPS)
LOAD_TARGET  = $(LOAD_SOURCES:.cpp=.o)

all: leaderboard produce_one monitor load

leaderboard: $(LEADERBOARD_TARGET)
	@echo "-----------------------"
	@echo "Make $(LEADERBOARD_SOURCES) with $(LIBS)"
	@mkdir -p bin
	@$(CXX) $(CPPFLAGS) $(LIBS) $(LEADERBOARD_SOURCES) -o bin/leaderboard
	@echo "Leaderboard location: bin/leaderboard. It is leaderboard binary - task result itself"
	
produce_one: $(PRODUCE_ONE_TARGET)
	@echo "-----------------------"
	@echo "Make $(PRODUCE_ONE_SOURCES) with $(LIBS)"
	@mkdir -p bin
	@$(CXX) $(CPPFLAGS) $(LIBS) $(PRODUCE_ONE_SOURCES) -o bin/produce_one
	@echo "produce_one location: bin/produce_one. It is binary to send one message to leaderboard"
	
monitor: $(MONITOR_TARGET)
	@echo "-----------------------"
	@echo "Make $(MONITOR_SOURCES) with $(LIBS)"
	@mkdir -p bin
	@$(CXX) $(CPPFLAGS) $(LIBS) $(MONITOR_SOURCES) -o bin/monitor
	@echo "monitor location: bin/monitor. It is binary to monitor leaderboard output messages"

load: $(LOAD_TARGET)
	@echo "-----------------------"
	@echo "Make $(LOAD_SOURCES) with $(LIBS)"
	@mkdir -p bin
	@$(CXX) $(CPPFLAGS) $(LIBS) $(LOAD_SOURCES) -o bin/load
	@echo "load location: bin/load. It is binary to produce some incoming load for leaderboard"	
	
clean:
	rm -rf bin/
	rm *.o