# Put this folder inside crow_server/. The existing crow_all.h stays one level up.
CXX ?= g++
CPPFLAGS += -I.. -I.
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pthread
LDLIBS += -pthread

fellowship_server: game_server.cpp game_routes.h game.h cards.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) game_server.cpp -o $@ $(LDLIBS)

test: tests/game_test.cpp game.h cards.h
	$(CXX) -std=c++17 -Wall -Wextra -I. tests/game_test.cpp -o /tmp/fellowship_game_test
	/tmp/fellowship_game_test
	$(CXX) -std=c++17 -Wall -Wextra -I. tests/simulation.cpp -o /tmp/fellowship_simulation
	/tmp/fellowship_simulation

clean:
	rm -f fellowship_server
.PHONY: test clean
