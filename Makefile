################################################################################
# Key paths and settings
################################################################################

SRCDIR = src
BUILDDIR = build
TARGET = bin/capture

################################################################################
# Dependencies
################################################################################

# Spinnaker deps
SPINNAKER_LIB = -l Spinnaker
SPINNAKER_INC = -isystem /usr/include/spinnaker # suppress spinnaker SDK warnings with `-isystem` include

# OpenCV deps
OPENCV_LIB = -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_imgproc -lopencv_core

################################################################################
# Master inc/lib/obj/dep settings
################################################################################

CFLAGS = -std=c++17 -Wall -D DEVELOPMENT -g3
CC = g++

SRCEXT = cpp
SOURCES = $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS = $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))

INC = -isystem lib -I include $(SPINNAKER_INC) -I /usr/include/opencv4
LIB = $(OPENCV_LIB) $(SPINNAKER_LIB) -Wl,-Bdynamic -pthread

################################################################################
# Rules/recipes
################################################################################

capture: $(OBJECTS)
	@mkdir -p $(dir $(TARGET))
	@echo " Linking..."
	@echo " $(CC) $^ -o $(TARGET) $(LIB)"; $(CC) $^ -o $(TARGET) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

# Clean up intermediate objects
clean_obj:
	rm -f $(OBJECTS_ALL)
	@echo "intermediate objects cleaned up!"

# Clean up everything.
clean: clean_obj
	rm -f $(TARGET)
	@echo "all cleaned up!"
