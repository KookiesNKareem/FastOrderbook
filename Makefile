CXX = g++
CXXFLAGS = -std=c++20 -O3 -march=native -ffast-math -fno-exceptions
TARGET = main

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) main.cpp $(LDFLAGS) $(LIBS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

profile: main.cpp
	$(CXX) $(CXXFLAGS) -pg main.cpp -o $(TARGET)_profile