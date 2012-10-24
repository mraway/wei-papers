DEDUP_SIM_LRU	= dedup_sim_lru
DEDUP_CDS_SIM	= dedup_cds_sim
DEDUP_ANALYSIS	= dedup_analysis
EB_SEGMENT	= eb_segment
EB_DEDUP	= eb_dedup
BINS		= $(DEDUP_SIM_LRU) $(DEDUP_CDS_SIM) $(EB_SEGMENT) $(EB_DEDUP) $(DEDUP_ANALYSIS)
HDRS		= dedup_types.hpp lru_cache.hpp
DEDUP_SIM_LRU_OBJS	= dedup_sim_lru.o lru_cache.o
DEDUP_CDS_SIM_OBJS	= dedup_cds_sim.o
DEDUP_ANALYSIS_OBJS	= dedup_analysis.o
EB_SEGMENT_OBJS	= eb_segment.o
EB_DEDUP_OBJS	= eb_dedup.o

CFLAGS		= -g -Wall
CPPFLAGS	= -g -Wall
LDFLAGS		= -lssl -lstdc++
CC 		= gcc

all:		$(DEDUP_CDS_SIM) $(DEDUP_SIM_LRU) $(DEDUP_ANALYSIS) $(EB_SEGMENT) $(EB_DEDUP)

$(DEDUP_SIM_LRU): $(DEDUP_SIM_LRU_OBJS)	

$(DEDUP_CDS_SIM): $(DEDUP_CDS_SIM_OBJS)

$(DEDUP_ANALYSIS): $(DEDUP_ANALYSIS_OBJS)

$(EB_SEGMENT):	$(EB_SEGMENT_OBJS)

$(EB_DEDUP):	$(EB_DEDUP_OBJS)

$(DEDUP_SIM_LRU_OBJS) $(DEDUP_CDS_SIM_OBJS) $(DEDUP_ANALYSIS_OBJS) $(EB_SEGMENT_OBJS) $(EB_DEDUP_OBJS):	$(HDRS)

clean:
	rm -f $(BINS) *.o
