#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <frame.h>

void
_frdrawtext(Frame *f, Point pt, Image *text, Image *back)
{
	Frbox *b;
	int nb;

	for(nb=0,b=f->box; nb<f->nbox; nb++, b++){
		_frcklinewrap(f, &pt, b);
		if(!f->noredraw && b->nrune >= 0)
			stringbg(f->b, pt, text, ZP, f->font, (char*)b->ptr, back, ZP);
		pt.x += b->wid;
	}
}

static int
nbytes(char *s0, int nr)
{
	char *s;
	Rune r;

	s = s0;
	while(--nr >= 0)
		s += chartorune(&r, s);
	return s-s0;
}

/*
 * stylecols_for — return the NCOL colour set for style index idx.
 * Falls back to f->cols when the index is out of range.
 */
static Image**
stylecols_for(Frame *f, int idx)
{
	if(f->stylecols == nil || idx < 0 || idx >= f->nstylecols)
		return f->cols;
	return f->stylecols + idx * NCOL;
}

/*
 * styleat — return the style index that covers character position p,
 * given the frame's run-length encoded styles array.
 * Also sets *segend to the first position NOT covered by this style run.
 */
static int
styleat(Frame *f, ulong p, ulong *segend)
{
	int i;
	ulong pos, len;

	pos = 0;
	for(i = 0; i < f->nstyles; i++){
		len = f->styles[i*2 + 1];
		if(p < pos + len){
			*segend = pos + len;
			return (int)f->styles[i*2];
		}
		pos += len;
	}
	/* past all explicit style runs → default style 0 */
	*segend = f->nchars;
	return 0;
}

/*
 * frdrawrange — draw characters [p0,p1) honouring both per-character styles
 * (f->styles / f->stylecols) and the current selection (f->p0, f->p1).
 * pt must be the screen point for p0.  Returns the screen point after p1.
 *
 * This is the single authoritative drawing primitive; frdrawsel and frredraw
 * are both implemented on top of it.
 */
Point
frdrawrange(Frame *f, Point pt, ulong p0, ulong p1)
{
	ulong p, segend, next;
	Image **sc;
	Image *back, *text;

	p = p0;
	while(p < p1){
		/* find the end of the current style run */
		if(f->nstyles > 0 && f->styles != nil){
			int sidx = styleat(f, p, &segend);
			if(segend > p1)
				segend = p1;
			sc = stylecols_for(f, sidx);
		} else {
			segend = p1;
			sc = f->cols;
		}

		/* split further at selection boundaries */
		while(p < segend){
			next = segend;
			if(f->p0 > p && f->p0 < next)
				next = f->p0;
			if(f->p1 > p && f->p1 < next)
				next = f->p1;

			if(p >= f->p0 && p < f->p1){
				back = sc[HIGH];
				text = sc[HTEXT];
			} else {
				back = sc[BACK];
				text = sc[TEXT];
			}
			pt = frdrawsel0(f, pt, p, next, back, text);
			p = next;
		}
	}
	return pt;
}

void
frdrawsel(Frame *f, Point pt, ulong p0, ulong p1, int issel)
{
	ulong p, segend;
	Image **sc;
	Image *back, *text;

	if(f->ticked)
		frtick(f, frptofchar(f, f->p0), 0);

	if(p0 == p1){
		frtick(f, pt, issel);
		return;
	}

	if(f->nstyles == 0 || f->styles == nil){
		/* fast path: no per-character styles */
		if(issel){
			back = f->cols[HIGH];
			text = f->cols[HTEXT];
		} else {
			back = f->cols[BACK];
			text = f->cols[TEXT];
		}
		frdrawsel0(f, pt, p0, p1, back, text);
		return;
	}

	/* style-aware: iterate style segments, apply issel uniformly within each */
	p = p0;
	while(p < p1){
		int sidx = styleat(f, p, &segend);
		if(segend > p1)
			segend = p1;
		sc = stylecols_for(f, sidx);
		if(issel){
			back = sc[HIGH];
			text = sc[HTEXT];
		} else {
			back = sc[BACK];
			text = sc[TEXT];
		}
		pt = frdrawsel0(f, pt, p, segend, back, text);
		p = segend;
	}
}

Point
frdrawsel0(Frame *f, Point pt, ulong p0, ulong p1, Image *back, Image *text)
{
	Frbox *b;
	int nb, nr, w, x, trim;
	Point qt;
	uint p;
	char *ptr;

	if(p0 > p1)
		sysfatal("libframe: frdrawsel0 p0=%lud > p1=%lud", p0, p1);

	p = 0;
	b = f->box;
	trim = 0;
	for(nb=0; nb<f->nbox && p<p1; nb++){
		nr = b->nrune;
		if(nr < 0)
			nr = 1;
		if(p+nr <= p0)
			goto Continue;
		if(p >= p0){
			qt = pt;
			_frcklinewrap(f, &pt, b);
			/* fill in the end of a wrapped line */
			if(pt.y > qt.y)
				draw(f->b, Rect(qt.x, qt.y, f->r.max.x, pt.y), back, nil, qt);
		}
		ptr = (char*)b->ptr;
		if(p < p0){	/* beginning of region: advance into box */
			ptr += nbytes(ptr, p0-p);
			nr -= (p0-p);
			p = p0;
		}
		trim = 0;
		if(p+nr > p1){	/* end of region: trim box */
			nr -= (p+nr)-p1;
			trim = 1;
		}
		if(b->nrune<0 || nr==b->nrune)
			w = b->wid;
		else
			w = stringnwidth(f->font, ptr, nr);
		x = pt.x+w;
		if(x > f->r.max.x)
			x = f->r.max.x;
		draw(f->b, Rect(pt.x, pt.y, x, pt.y+f->font->height), back, nil, pt);
		if(b->nrune >= 0)
			stringnbg(f->b, pt, text, ZP, f->font, ptr, nr, back, ZP);
		pt.x += w;
	    Continue:
		b++;
		p += nr;
	}
	/* if this is end of last plain text box on wrapped line, fill to end of line */
	if(p1>p0 &&  b>f->box && b<f->box+f->nbox && b[-1].nrune>0 && !trim){
		qt = pt;
		_frcklinewrap(f, &pt, b);
		if(pt.y > qt.y)
			draw(f->b, Rect(qt.x, qt.y, f->r.max.x, pt.y), back, nil, qt);
	}
	return pt;
}

void
frredraw(Frame *f)
{
	int ticked;
	Point pt;

	ticked = f->ticked;
	if(ticked)
		frtick(f, frptofchar(f, f->p0), 0);

	pt = frptofchar(f, 0);
	frdrawrange(f, pt, 0, f->nchars);

	if(ticked)
		frtick(f, frptofchar(f, f->p0), 1);
	else if(f->p0 == f->p1)
		frtick(f, frptofchar(f, f->p0), 1);
}

static void
_frtick(Frame *f, Point pt, int ticked)
{
	Rectangle r;

	if(f->ticked==ticked || f->tick==0 || !ptinrect(pt, f->r))
		return;
	pt.x -= f->tickscale;	/* looks best just left of where requested */
	r = Rect(pt.x, pt.y, pt.x+FRTICKW*f->tickscale, pt.y+f->font->height);
	/* can go into left border but not right */
	if(r.max.x > f->r.max.x)
		r.max.x = f->r.max.x;
	if(ticked){
		draw(f->tickback, f->tickback->r, f->b, nil, pt);
		draw(f->b, r, f->tick, nil, ZP);
	}else
		draw(f->b, r, f->tickback, nil, ZP);
	f->ticked = ticked;
}

void
frtick(Frame *f, Point pt, int ticked)
{
	if(f->tickscale != scalesize(f->display, 1)) {
		if(f->ticked)
			_frtick(f, pt, 0);
		frinittick(f);
	}
	_frtick(f, pt, ticked);
}

Point
_frdraw(Frame *f, Point pt)
{
	Frbox *b;
	int nb, n;

	for(b=f->box,nb=0; nb<f->nbox; nb++, b++){
		_frcklinewrap0(f, &pt, b);
		if(pt.y == f->r.max.y){
			f->nchars -= _frstrlen(f, nb);
			_frdelbox(f, nb, f->nbox-1);
			break;
		}
		if(b->nrune > 0){
			n = _frcanfit(f, pt, b);
			if(n == 0)
				break;
			if(n != b->nrune){
				_frsplitbox(f, nb, n);
				b = &f->box[nb];
			}
			pt.x += b->wid;
		}else{
			if(b->bc == '\n'){
				pt.x = f->r.min.x;
				pt.y+=f->font->height;
			}else
				pt.x += _frnewwid(f, pt, b);
		}
	}
	return pt;
}

int
_frstrlen(Frame *f, int nb)
{
	int n;

	for(n=0; nb<f->nbox; nb++)
		n += NRUNE(&f->box[nb]);
	return n;
}
