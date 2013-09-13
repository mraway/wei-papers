CFLAGS		= -O2 -Wall
CPPFLAGS	= -O2 -Wall
LDLIBS		= -lssl -lcrypto -lstdc++
CC 		= clang
CXX		= clang++

BINS = dedup_cds_sim dedup_cds_gen dedup_sort dedup_analysis eb_segment eb_dedup cds_hit_analysis
LRU_BINS = dedup_cds_lru dedup_sim_lru
HDRS = dedup_types.hpp
LRU_LIBS = lru_cache
OBJS = $(addsuffix .o,$(BINS))
LRU_OBJS = $(addsuffix .o,$(LRU_BINS))
LRU_LIB_OBJS = $(addsuffix .o,$(LRU_LIBS))
LRU_LIB_HDRS = $(addsuffix .hpp,$(LRU_LIBS))

all: $(BINS) $(LRU_BINS)

$(BINS) $(LRU_BINS): %:%.o

$(LRU_BINS): $(LRU_LIB_OBJS)

$(OBJS) $(LRU_OBJS) $(LRU_LIB_OBJS): $(HDRS)

$(LRU_LIB_OBJS): $(LRU_LIB_HDRS)

clean:
	rm -f $(BINS) $(LRU_BINS) *.o
