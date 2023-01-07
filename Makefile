# --- MACROS
# Define program name
MAIN= improc

# Define the C compiler to use
CC= gcc

# Define any compile-time flags
CFLAGS= -Wall -pedantic

# Use this flag if you want to compile without the image viewers.
ifdef NOVIEW
	CFLAGS += -DNOVIEW=1
endif

# Use this flag if you want to disable printing warnings to the console.
ifdef DISABLE_WARNINGS
	CFLAGS += -DDISABLE_WARNINGS=1
endif

# Make with "make RELEASE=1" for release build
ifndef RELEASE
	CFLAGS+= -g -O2
else
	CFLAGS+= -O3 -DFAST
endif

# Define any libraries to link into executable
LIBS= -lm

ifndef NOVIEW
  LIBS += -lglut -lX11

	OS := $(shell uname)
	ifeq ($(OS),Darwin)
		LIBS += -framework OpenGL
	else
		LIBS += -lGL  -lGLU
		# check for Linux and run other commands
	endif
endif

# Define C source files
SRCS= ${wildcard src/*.c} ${wildcard src/**/*.c}

# Define C header files
HDRS= ${wildcard src/*.h} ${wildcard src/**/*.h}

# Remove the image viewers from the source files
ifdef NOVIEW
	TMPSRC := $(SRCS)
	SRCS = $(filter-out src/greyimviewer.c,$(TMPSRC))
	TMPSRC := $(SRCS)
	SRCS = $(filter-out  src/rgbimviewer.c,$(TMPSRC))
	TMPSRC := $(SRCS)
	SRCS = $(filter-out src/greyimviewer.h,$(TMPSRC))
	TMPSRC := $(SRCS)
	SRCS = $(filter-out  src/rgbimviewer.h,$(TMPSRC))
endif

# --- TARGETS
all: ${MAIN}

# Builds the program
${MAIN}: ${SRCS} ${HDRS}
	@echo #
	@echo "-- BUILDING PROGRAM --"
	${CC} ${SRCS} ${CFLAGS} ${LIBS} -o ${MAIN}

clean:
	@echo #
	@echo "-- CLEANING PROJECT FILES --"
	$(RM) *.o ${MAIN}
