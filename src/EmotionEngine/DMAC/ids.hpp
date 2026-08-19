#ifndef EMOTIONENGINE_DMAC_IDS_HPP
#define EMOTIONENGINE_DMAC_IDS_HPP

namespace EmotionEngine::DMA {
enum class SourceChainTagID {
/*```
MADR = DMAtag.ADDR;
TADR += 16;
tag_end = true;
```*/
	refe,

/*```
MADR = TADR + 16;
TADR = MADR;
```*/
	cnt,

/*```
MADR = TADR + 16;
TADR = DMAtag.ADDR;
```*/
	next,

/*```
MADR = DMAtag.ADDR;
TADR += 16;
```*/
	ref,

/*```
MADR = DMAtag.ADDR;
TADR += 16;
```*/
	refs,

/*```
MADR = TADR + 16;

// ASP == 0
if (CHCR.ASP == 0) {
ASR0 = MADR + (QWC * 16);
}

// ASP == 1
else if (CHCR.ASP == 1) {
ASR1 = MADR + (QWC * 16);
}

TADR = DMAtag.ADDR;
CHCR.ASP++;
```*/
	call,

/*```
MADR = TADR + 16;

// ASP == 2
if (CHCR.ASP == 2) {
TADR = ASR1;
CHCR.ASP--;
}

// ASP == 1
else if (CHCR.ASP == 1) {
TADR = ASR0;
CHCR.ASP--;
}

// end tag
else {
tag_end = true;
}
```*/
	ret,

/*```
MADR = TADR + 16;
tag_end = true;
```*/
	end
};

enum class DestChainTagID {
/*```
MADR = DMAtag.ADDR;
```*/
	cnt,

/*```
MADR = DMAtag.ADDR;
```*/
	cnts,

/*```
MADR = DMAtag.ADDR;
tag_end = true;
```*/
	end = 7
};
}

#endif