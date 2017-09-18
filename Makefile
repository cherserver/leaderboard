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

all: leaderboard produce_one

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
	