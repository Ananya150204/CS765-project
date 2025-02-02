# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++23

# Target executable name
TARGET = simulator
OBJDIR = build
SRC = src

# List of source files (using a wildcard)
SRCS = $(wildcard $(SRC)/*.cpp)

# Convert source files to object files stored in OBJDIR
OBJS = $(patsubst $(SRC)/%.cpp, $(OBJDIR)/%.o, $(SRCS))

# Default rule
all: $(TARGET)

# Link the executable from the object files
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

# Pattern rule: compile .cpp files to object files in OBJDIR.
# The pipe symbol (|) indicates that $(OBJDIR) is an order-only prerequisite.
$(OBJDIR)/%.o: $(SRC)/%.cpp $(SRC)/declarations.h | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ensure that the OBJDIR directory exists
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Clean rule to remove the executable and object files
clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf $(OBJDIR)
