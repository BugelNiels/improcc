# --- MACROS
# define program name
MAIN= improc

# define the C compiler to use
CC= gcc

# define any compile-time flags
# For debugging purposes you can add the -g flag here
# Additionally, if you want to increase the performance of the framework, add the "-DFAST" flag here.
# Note that this disables domain and dynamic range checks!
CFLAGS= -Wall -pedantic

# Make with "make RELEASE=1" for release build
ifndef RELEASE
	CFLAGS+= -g -O2
else
	CFLAGS+= -O3 -DFAST
endif

# define any libraries to link into executable
LIBS= -lm -lglut -lX11

OS := $(shell uname)
ifeq ($(OS),Darwin)
  LIBS += -framework OpenGL
else
	LIBS += -lGL  -lGLU
  # check for Linux and run other commands
endif

# define C source files
SRCS= ${wildcard src/*.c} ${wildcard src/**/*.c}

# define C header files
HDRS= ${wildcard src/*.h} ${wildcard src/**/*.h}

# --- TARGETS
all: ${MAIN}

#Builds the program
${MAIN}: ${SRCS} ${HDRS}
	@echo #
	@echo "-- BUILDING PROGRAM --"
	${CC} ${SRCS} ${CFLAGS} ${LIBS} -o ${MAIN}

clean:
	@echo #
	@echo "-- CLEANING PROJECT FILES --"
	$(RM) *.o ${MAIN}