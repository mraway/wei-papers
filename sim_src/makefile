EB_SEGMENT	= eb_segment
EB_DEDUP	= eb_dedup
BINS		= $(EB_SEGMENT) $(EB_DEDUP)
HDRS		= dedup_types.hpp
EB_SEGMENT_OBJS	= eb_segment.o
EB_DEDUP_OBJS	= eb_dedup.o

CFLAGS		= -O2 -Wall
CPPFLAGS	= -O2 -Wall
LDFLAGS		= -lssl -lstdc++
CC 		= gcc

all:		$(EB_SEGMENT) $(EB_DEDUP)

$(EB_SEGMENT):	$(EB_SEGMENT_OBJS)

$(EB_DEDUP):	$(EB_DEDUP_OBJS)

$(EB_SEGMENT_OBJS) $(EB_DEDUP_OBJS):	$(HDRS)

clean:
	rm -f $(BINS) *.o
