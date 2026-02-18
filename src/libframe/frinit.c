#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <frame.h>

void
frinit(Frame *f, Rectangle r, Font *ft, Image *b, Image *cols[NCOL])
{
	f->font = ft;
	f->display = b->display;
	f->maxtab = 8*stringwidth(ft, "0");
	f->nbox = 0;
	f->nalloc = 0;
	f->nchars = 0;
	f->nlines = 0;
	f->p0 = 0;
	f->p1 = 0;
	f->box = 0;
	f->lastlinefull = 0;
	f->nstylecols = 0;
	f->stylecols = nil;
	f->nstyles = 0;
	f->styles = nil;
	if(cols != 0)
		memmove(f->cols, cols, sizeof f->cols);
	frsetrects(f, r, b);
	if(f->tick==nil && f->cols[BACK]!=0)
		frinittick(f);
}

void
frinittick(Frame *f)
{
	Image *b;
	Font *ft;

	if(f->cols[BACK] == nil || f->display == nil)
		return;
	f->tickscale = scalesize(f->display, 1);
	b = f->display->screenimage;
	ft = f->font;
	if(f->tick)
		freeimage(f->tick);
	f->tick = allocimage(f->display, Rect(0, 0, f->tickscale*FRTICKW, ft->height), b->chan, 0, DWhite);
	if(f->tick == nil)
		return;
	if(f->tickback)
		freeimage(f->tickback);
	f->tickback = allocimage(f->display, f->tick->r, b->chan, 0, DWhite);
	if(f->tickback == 0){
		freeimage(f->tick);
		f->tick = 0;
		return;
	}
	/* background color */
	draw(f->tick, f->tick->r, f->cols[BACK], nil, ZP);
	/* vertical line */
	draw(f->tick, Rect(f->tickscale*(FRTICKW/2), 0, f->tickscale*(FRTICKW/2+1), ft->height), f->cols[TEXT], nil, ZP);
	/* box on each end */
	draw(f->tick, Rect(0, 0, f->tickscale*FRTICKW, f->tickscale*FRTICKW), f->cols[TEXT], nil, ZP);
	draw(f->tick, Rect(0, ft->height-f->tickscale*FRTICKW, f->tickscale*FRTICKW, ft->height), f->cols[TEXT], nil, ZP);
}

void
frsetrects(Frame *f, Rectangle r, Image *b)
{
	f->b = b;
	f->entire = r;
	f->r = r;
	f->r.max.y -= (r.max.y-r.min.y)%f->font->height;
	f->maxlines = (r.max.y-r.min.y)/f->font->height;
}

void
frclear(Frame *f, int freeall)
{
	if(f->nbox)
		_frdelbox(f, 0, f->nbox-1);
	if(f->box)
		free(f->box);
	/* style arrays are always freed; winframesync re-applies them after reinit */
	free(f->stylecols);
	f->stylecols = nil;
	f->nstylecols = 0;
	free(f->styles);
	f->styles = nil;
	f->nstyles = 0;
	if(freeall){
		freeimage(f->tick);
		freeimage(f->tickback);
		f->tick = 0;
		f->tickback = 0;
	}
	f->box = 0;
	f->ticked = 0;
}

/*
 * frsetstyles — install a new style table and run-length colour array into a
 * frame.  The caller owns the memory; Frame keeps a fresh copy.
 *
 *   nsc   — number of style colour sets
 *   sc    — flat array of nsc*NCOL Image* pointers (sc[0..NCOL-1] == cols)
 *   ns    — number of run-length style segments
 *   st    — flat array of 2*ns ulongs: {style_index, length, ...}
 *
 * Returns 1 on success, 0 on allocation failure.
 */
int
frsetstyles(Frame *f, int nsc, Image **sc, int ns, ulong *st)
{
	Image **newsc;
	ulong  *newst;

	newsc = nil;
	newst = nil;
	if(nsc > 0 && sc != nil){
		newsc = malloc(nsc * NCOL * sizeof(Image*));
		if(newsc == nil)
			return 0;
		memmove(newsc, sc, nsc * NCOL * sizeof(Image*));
	}
	if(ns > 0 && st != nil){
		newst = malloc(2 * ns * sizeof(ulong));
		if(newst == nil){
			free(newsc);
			return 0;
		}
		memmove(newst, st, 2 * ns * sizeof(ulong));
	}
	free(f->stylecols);
	free(f->styles);
	f->stylecols  = newsc;
	f->nstylecols = nsc;
	f->styles     = newst;
	f->nstyles    = ns;
	/* keep cols[0..NCOL-1] in sync with style 0 */
	frstylesync(f);
	return 1;
}

/*
 * frstylesync — copy style 0's colours back into f->cols so that all
 * existing libframe drawing helpers see the correct default palette.
 */
void
frstylesync(Frame *f)
{
	if(f->nstylecols > 0 && f->stylecols != nil)
		memmove(f->cols, f->stylecols, NCOL * sizeof(Image*));
}

/*
 * frstyleinsert — widen the RLE style run that covers position p0 by n
 * characters.  Newly inserted characters inherit the style of the character
 * currently at p0 (i.e. the right neighbour), so typing inside a highlighted
 * region keeps the highlight.
 */
void
frstyleinsert(Frame *f, ulong p0, ulong n)
{
	ulong pos, segend;
	int i;

	if(f->nstyles == 0 || n == 0)
		return;
	pos = 0;
	for(i = 0; i < f->nstyles; i++){
		segend = pos + f->styles[2*i+1];
		if(p0 <= segend || i == f->nstyles-1){
			/* p0 falls within or at the end of this segment — extend it.
			 * Using <= means the boundary belongs to the left neighbour:
			 * typing at the end of a styled run continues that style. */
			f->styles[2*i+1] += n;
			return;
		}
		pos = segend;
	}
}

/*
 * frstyledelete — remove n = p1-p0 characters starting at p0 from the RLE
 * style array, shrinking or eliminating the affected segments.
 */
void
frstyledelete(Frame *f, ulong p0, ulong p1)
{
	ulong pos, del_start, seglen, avail, del, rem;
	int i;

	if(f->nstyles == 0 || p0 >= p1)
		return;
	rem = p1 - p0;
	pos = 0;
	/* find the first segment that overlaps [p0, p1) */
	for(i = 0; i < f->nstyles; i++){
		if(pos + f->styles[2*i+1] > p0)
			break;
		pos += f->styles[2*i+1];
	}
	if(i == f->nstyles)
		return;			/* p0 is beyond all segments */
	del_start = p0 - pos;		/* offset within segment i where deletion begins */
	while(i < f->nstyles && rem > 0){
		seglen = f->styles[2*i+1];
		avail  = seglen - del_start;
		del    = rem < avail ? rem : avail;
		f->styles[2*i+1] -= del;
		rem      -= del;
		del_start = 0;		/* subsequent segments delete from offset 0 */
		if(f->styles[2*i+1] == 0){
			/* segment is fully consumed — remove it */
			memmove(&f->styles[2*i], &f->styles[2*(i+1)],
				2*(f->nstyles-i-1)*sizeof(ulong));
			f->nstyles--;
			/* i now points to the next segment; don't increment */
		} else {
			i++;
		}
	}
}
