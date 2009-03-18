BZIP2.SOURCE=\
	bzip2/blocksort.c \
	bzip2/huffman.c \
	bzip2/crctable.c \
	bzip2/randtable.c \
	bzip2/compress.c \
	bzip2/decompress.c \
	bzip2/bzip2.c \
	bzip2/bzlib.c

BZIP2.SOURCE.OBJ=$(addprefix $(DIR.OBJ)/, $(BZIP2.SOURCE:.c=$O))
SOURCE.ALL += $(BZIP2.SOURCE)
targetsclean += TORQUEclean

DIR.LIST = $(addprefix $(DIR.OBJ)/, $(sort $(dir $(SOURCE.ALL))))

$(DIR.LIST): targets.bzip2.mk

$(DIR.OBJ)/bzip2$(EXT.LIB): CFLAGS+=-Ibzip2 
$(DIR.OBJ)/bzip2$(EXT.LIB): $(DIR.LIST) $(BZIP2.SOURCE.OBJ)
	$(DO.LINK.LIB)

bzip2clean:
ifneq ($(wildcard DEBUG.*),)
	-$(RM)  DEBUG*
endif
ifneq ($(wildcard RELEASE.*),)
	-$(RM)  RELEASE*
endif
