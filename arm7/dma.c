
static  void 
dmaCopyWords(uint8 channel, const void* src, void* dest, uint32 size) 
{
	DMA_SRC(channel) = (uint32)src;
	DMA_DEST(channel) = (uint32)dest;
	DMA_CR(channel) = DMA_COPY_WORDS | (size>>2);
	while(DMA_CR(channel) & DMA_BUSY);
}

static  void dmaCopyHalfWords(uint8 channel, const void* src, void* dest, uint32 size) {
	DMA_SRC(channel) = (uint32)src;
	DMA_DEST(channel) = (uint32)dest;
	DMA_CR(channel) = DMA_COPY_HALFWORDS | (size>>1);
	while(DMA_CR(channel) & DMA_BUSY);
}

static  void dmaCopy(const void * source, void * dest, uint32 size) {
	DMA_SRC(3) = (uint32)source;
	DMA_DEST(3) = (uint32)dest;
	DMA_CR(3) = DMA_COPY_HALFWORDS | (size>>1);
	while(DMA_CR(3) & DMA_BUSY);
}

static  void dmaCopyWordsAsynch(uint8 channel, const void* src, void* dest, uint32 size) {
	DMA_SRC(channel) = (uint32)src;
	DMA_DEST(channel) = (uint32)dest;
	DMA_CR(channel) = DMA_COPY_WORDS | (size>>2);

}

static  void dmaCopyHalfWordsAsynch(uint8 channel, const void* src, void* dest, uint32 size) {
	DMA_SRC(channel) = (uint32)src;
	DMA_DEST(channel) = (uint32)dest;
	DMA_CR(channel) = DMA_COPY_HALFWORDS | (size>>1);
}

static  void dmaCopyAsynch(const void * source, void * dest, uint32 size) {
	DMA_SRC(3) = (uint32)source;
	DMA_DEST(3) = (uint32)dest;
	DMA_CR(3) = DMA_COPY_HALFWORDS | (size>>1);
}

static  void dmaFillWords( const void* src, void* dest, uint32 size) {
	DMA_SRC(3) = (uint32)src;
	DMA_DEST(3) = (uint32)dest;
	DMA_CR(3) = DMA_SRC_FIX | DMA_COPY_WORDS | (size>>2);
	while(DMA_CR(3) & DMA_BUSY);
}

static  void dmaFillHalfWords( const void* src, void* dest, uint32 size) {
	DMA_SRC(3) = (uint32)src;
	DMA_DEST(3) = (uint32)dest;
	DMA_CR(3) = DMA_SRC_FIX | DMA_COPY_HALFWORDS | (size>>1);
	while(DMA_CR(3) & DMA_BUSY);
}

static  int dmaBusy(uint8 channel) {
	return (DMA_CR(channel) & DMA_BUSY)>>31;
}