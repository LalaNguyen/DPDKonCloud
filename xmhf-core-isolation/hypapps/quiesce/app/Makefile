#-------------------------------------------------------------------------------
# makefile for XMHF apps
# author: amit vasudevan (amitvasudevan@acm.org)

# source files
AS_SOURCES = 
C_SOURCES = $(wildcard *.c)

OBJECTS = $(patsubst %.S, $(APPOBJECTSDIR)/%.o, $(AS_SOURCES))
OBJECTS += $(patsubst %.c, $(APPOBJECTSDIR)/%.o, $(C_SOURCES))

I_SOURCES  =  $(wildcard $(EMHF_INCLUDE)/*.h)
I_SOURCES  +=  $(wildcard $(APP_INCLUDEDIR)/*.h)

# targets
.PHONY: all
all: $(TARGET_ARCHIVE)

$(TARGET_ARCHIVE): $(OBJECTS)
	mkdir -p $(APPOBJECTSDIR)
	$(AR) -rcs $(TARGET_ARCHIVE) $(OBJECTS)

$(APPBOJECTSDIR)/%.o: %.S $(I_SOURCES) Makefile ../Makefile 
	$(CC) -c $(ASFLAGS) -o $@ $<
$(APPOBJECTSDIR)/%.o: %.c $(I_SOURCES) Makefile ../Makefile
	$(CC) -c $(CFLAGS) -o $@ $<

.PHONY: clean 
clean:
	$(RM) -f $(OBJECTS) $(TARGET_ARCHIVE)

  
# Minh: Need to manually compile the S file so that it will save in the objects folder