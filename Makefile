CXX= g++
CXXFLAGS= -std=c++11 -D_VARIADIC_MAX=10

INCLUDE= -I./include
LIB= -lpthread

SRCDIR= src
OBJDIR= obj
BINDIR= bin

OBJS= $(addprefix $(OBJDIR)/, main.o configreader.o process.o)
EXEC= $(addprefix $(BINDIR)/, osscheduler)

# CREATE DIRECTORIES (IF DON'T ALREADY EXIST)
mkdirs:= $(shell mkdir -p $(OBJDIR) $(BINDIR))


# BUILD EVERYTHING
all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIB)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INCLUDE)


# REMOVE OLD FILES
clean:
	rm -f $(OBJS) $(EXEC)
