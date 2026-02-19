#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <cursor.h>
#include <mouse.h>
#include <keyboard.h>
#include <frame.h>
#include <fcall.h>
#include <plumb.h>
#include <libsec.h>
#include "dat.h"
#include "fns.h"

/*
 * loadstylefile — parse a style file and populate the global styles table.
 *
 * File format (lines beginning with '#' are ignored):
 *
 *   default  <back> <high> <border> <text> <htext>     (5 colour fields)
 *   <name>   <back> <high> <text> <htext>               (4 colour fields)
 *
 * Each colour value is a 32-bit RGBA hex literal, e.g. 0xFFFFFFFF.
 * The "default" entry (index 0) replaces the hard-coded colours in Acme.
 *
 * Returns 1 on success, 0 on failure.
 */

static Image*
colorimage(ulong rgba)
{
	return allocimage(display, Rect(0,0,1,1), screen->chan, 1, rgba);
}

int
loadstylefile(char *path)
{
	int fd, nlines, i;
	char *data, *p, *q, *end;
	long n;
	Style *newstyles, *s;
	Dir *d;

	fd = open(path, OREAD);
	if(fd < 0)
		return 0;

	d = dirfstat(fd);
	if(d == nil){
		close(fd);
		return 0;
	}
	n = d->length;
	free(d);

	data = malloc(n+1);
	if(data == nil){
		close(fd);
		return 0;
	}
	if(readn(fd, data, n) != n){
		free(data);
		close(fd);
		return 0;
	}
	close(fd);
	data[n] = '\0';

	/* count non-comment, non-empty lines to size the array */
	nlines = 0;
	for(p = data; *p; ){
		q = strchr(p, '\n');
		if(q == nil)
			q = p + strlen(p);
		/* skip leading whitespace */
		while(p < q && (*p == ' ' || *p == '\t'))
			p++;
		if(p < q && *p != '#')
			nlines++;
		p = (*q == '\n') ? q+1 : q;
	}

	if(nlines == 0){
		free(data);
		return 0;
	}

	newstyles = emalloc(nlines * sizeof(Style));
	memset(newstyles, 0, nlines * sizeof(Style));

	i = 0;
	end = data + n;
	p = data;
	while(p < end && i < nlines){
		/* find end of line */
		q = memchr(p, '\n', end - p);
		if(q == nil)
			q = end;
		*q = '\0';

		/* skip leading whitespace */
		char *line = p;
		while(*line == ' ' || *line == '\t')
			line++;
		p = q + 1;

		/* skip blank lines and comments */
		if(*line == '\0' || *line == '#')
			continue;

		s = &newstyles[i];

		/* parse name */
		char *tok = line;
		char *sp = strpbrk(tok, " \t");
		if(sp == nil)
			continue;	/* no colour fields */
		*sp = '\0';
		s->name = estrdup(tok);
		tok = sp + 1;
		while(*tok == ' ' || *tok == '\t')
			tok++;

		/*
		 * Parse colour fields.
		 * "default" has 5 fields: back high border text htext
		 * All other entries have 4 fields: back high text htext
		 */
		int isdefault = (strcmp(s->name, "default") == 0);
		int nfields   = isdefault ? 5 : 4;
		ulong vals[5];
		int f;
		for(f = 0; f < nfields; f++){
			while(*tok == ' ' || *tok == '\t')
				tok++;
			if(*tok == '\0')
				break;
			char *ep;
			vals[f] = strtoul(tok, &ep, 0);
			tok = ep;
		}
		if(f < nfields)
			continue;	/* malformed line */

		if(isdefault){
			s->cols[BACK]  = colorimage(vals[0]);
			s->cols[HIGH]  = colorimage(vals[1]);
			s->cols[BORD]  = colorimage(vals[2]);
			s->cols[TEXT]  = colorimage(vals[3]);
			s->cols[HTEXT] = colorimage(vals[4]);
		} else {
			s->cols[BACK]  = colorimage(vals[0]);
			s->cols[HIGH]  = colorimage(vals[1]);
			s->cols[BORD]  = nil;	/* inherit from default */
			s->cols[TEXT]  = colorimage(vals[2]);
			s->cols[HTEXT] = colorimage(vals[3]);
		}

		i++;
	}
	free(data);

	/* Free old table */
	if(styles != nil){
		for(i = 0; i < nstyles; i++)
			free(styles[i].name);
		free(styles);
	}

	styles  = newstyles;
	nstyles = i;
	return 1;
}



/*
 * winframesync — apply the window's single style layer to the frame and repaint.
 *
 * w->styles/w->nstyles is a file-absolute {index, length} RLE.  We intersect
 * it with the visible window [org, org+nchars) to produce a frame-relative
 * RLE which is handed to frsetstyles, then repaint.
 *
 * Call after any style change, after a frame reinit, or when t->org changes.
 */
void
winframesync(Window *w)
{
	Frame *f;
	Text *t;
	Image **sc;
	ulong *frstyles, org, frnchars;
	int nfrstyles, i, j;
	ulong k;

	t = &w->body;
	f = &t->fr;
	org      = t->org;
	frnchars = (ulong)f->nchars;

	if(nstyles == 0 || w->nstyles == 0){
		frsetstyles(f, 0, nil, 0, nil);
		frredraw(f);
		flushimage(display, 1);
		return;
	}

	/* Build colour table: nstyles × NCOL */
	sc = emalloc(nstyles * NCOL * sizeof(Image*));
	for(i = 0; i < nstyles; i++)
		for(j = 0; j < NCOL; j++){
			Image *img = styles[i].cols[j];
			if(img == nil)
				img = f->cols[j];
			sc[i*NCOL+j] = img;
		}

	/* Expand file-absolute RLE into a per-character style index array. */
	ulong *composed = emalloc(frnchars * sizeof(ulong));
	memset(composed, 0, frnchars * sizeof(ulong));

	ulong filepos = 0;
	for(i = 0; i < w->nstyles; i++){
		ulong sidx    = w->styles[i*2];
		ulong slen    = w->styles[i*2+1];
		ulong seg_end = filepos + slen;
		if(sidx != 0){
			ulong from = filepos < org          ? org          : filepos;
			ulong to   = seg_end < org+frnchars ? seg_end      : org+frnchars;
			if(from < to){
				for(k = from - org; k < to - org; k++)
					composed[k] = sidx;
			}
		}
		filepos = seg_end;
		if(filepos >= org + frnchars)
			break;
	}

	/* RLE-compress composed[] into frstyles. */
	frstyles  = emalloc(2 * (frnchars + 1) * sizeof(ulong));
	nfrstyles = 0;
	if(frnchars > 0){
		ulong cur_idx = composed[0];
		ulong cur_len = 1;
		for(k = 1; k < frnchars; k++){
			if(composed[k] == cur_idx){
				cur_len++;
			} else {
				frstyles[nfrstyles*2]   = cur_idx;
				frstyles[nfrstyles*2+1] = cur_len;
				nfrstyles++;
				cur_idx = composed[k];
				cur_len = 1;
			}
		}
		frstyles[nfrstyles*2]   = cur_idx;
		frstyles[nfrstyles*2+1] = cur_len;
		nfrstyles++;
	}
	free(composed);

	frsetstyles(f, nstyles, sc, nfrstyles, frstyles);
	free(sc);
	free(frstyles);
	frredraw(f);
	flushimage(display, 1);
}

/*
 * winstyleinsert — keep w->styles in sync when n runes are inserted at q0.
 *
 * Widens the segment that covers q0 by n so that all absolute offsets after
 * q0 remain correct.  Also pokes the frame-level cache (f->styles) so
 * styleat() returns correct values immediately without a full winframesync.
 */
void
winstyleinsert(Window *w, uint q0, uint n)
{
	Text *t;
	Frame *f;
	int i;
	ulong pos;

	if(n == 0 || w->nstyles == 0)
		return;

	pos = 0;
	for(i = 0; i < w->nstyles; i++){
		if(q0 <= pos + w->styles[i*2+1]){
			w->styles[i*2+1] += n;
			break;
		}
		pos += w->styles[i*2+1];
	}

	t = &w->body;
	f = &t->fr;
	if(f->nstyles > 0 && (ulong)q0 >= t->org && (ulong)q0 < t->org + (ulong)f->nchars)
		frstyleinsert(f, (ulong)(q0 - t->org), (ulong)n);
}

/*
 * winstyledelete — keep w->styles in sync when runes [q0,q1) are removed.
 *
 * Shrinks/removes segments that overlap [q0,q1).  Segments after q1 are
 * implicitly shifted because their absolute position is the cumulative sum
 * of preceding lengths.
 */
void
winstyledelete(Window *w, uint q0, uint q1)
{
	int i, new_nsegs;
	ulong pos, seg_end, overlap;

	if(q0 >= q1 || w->nstyles == 0)
		return;

	pos       = 0;
	new_nsegs = 0;
	for(i = 0; i < w->nstyles; i++){
		ulong len = w->styles[i*2+1];
		seg_end   = pos + len;
		ulong ov0 = pos     > (ulong)q0 ? pos     : (ulong)q0;
		ulong ov1 = seg_end < (ulong)q1 ? seg_end : (ulong)q1;
		overlap   = ov1 > ov0 ? ov1 - ov0 : 0;
		if(len - overlap > 0){
			w->styles[new_nsegs*2]   = w->styles[i*2];
			w->styles[new_nsegs*2+1] = len - overlap;
			new_nsegs++;
		}
		pos = seg_end;
	}
	w->nstyles = new_nsegs;
}

/*
 * winclearstyle — free all style data for w.
 */
void
winclearstyle(Window *w)
{
	free(w->styles);
	w->styles  = nil;
	w->nstyles = 0;
}

/*
 * winsetstyle — splice new RLE segments into w->styles.
 *
 *   start   — character offset where the new segments begin
 *   newseg  — flat array of {style_index, length} pairs
 *   nnewseg — number of pairs (total ulongs = 2*nnewseg)
 *
 * If nnewseg == 0, clears all style data.
 */
void
winsetstyle(Window *w, ulong start, ulong *newseg, int nnewseg)
{
	ulong *old, *res;
	int nold, nres, j;
	ulong pos, newlen, end, covered;

	if(nnewseg == 0){
		winclearstyle(w);
		return;
	}

	old  = w->styles;
	nold = w->nstyles;

	newlen = 0;
	for(j = 0; j < nnewseg; j++)
		newlen += newseg[j*2+1];
	end = start + newlen;

	res  = emalloc(2 * (nold + nnewseg + 3) * sizeof(ulong));
	nres = 0;

	/* Phase 1: copy old segments entirely before 'start'; trim straddle. */
	covered = 0;
	pos = 0;
	for(j = 0; j < nold; j++){
		ulong sidx = old[j*2];
		ulong slen = old[j*2+1];
		ulong send = pos + slen;
		if(send <= start){
			res[nres*2]   = sidx;
			res[nres*2+1] = slen;
			nres++;
			covered = send;
		} else if(pos < start){
			res[nres*2]   = sidx;
			res[nres*2+1] = start - pos;
			nres++;
			covered = start;
		}
		pos += slen;
		if(pos >= start)
			break;
	}

	/* Gap fill if old array didn't reach 'start'. */
	if(covered < start){
		res[nres*2]   = 0;
		res[nres*2+1] = start - covered;
		nres++;
	}

	/* Phase 2: insert new segments. */
	for(j = 0; j < nnewseg; j++){
		res[nres*2]   = newseg[j*2];
		res[nres*2+1] = newseg[j*2+1];
		nres++;
	}

	/* Phase 3: copy old segments entirely after 'end'; trim straddle. */
	pos = 0;
	for(j = 0; j < nold; j++){
		ulong sidx = old[j*2];
		ulong slen = old[j*2+1];
		ulong send = pos + slen;
		if(send > end){
			ulong from = pos < end ? end : pos;
			res[nres*2]   = sidx;
			res[nres*2+1] = send - from;
			nres++;
		}
		pos += slen;
	}

	free(old);
	w->styles  = res;
	w->nstyles = nres;
}

/*
 * ctlstyleparse — parse and apply a "style" ctl command line.
 *
 * Format (p points past "style ", e is end of the line):
 *
 *   <index>                               "style 0" — clear all style data
 *   <index> <start> <length> [...]        one or more segments
 *
 * Returns an error string on failure, nil on success.
 */
char*
ctlstyleparse(Window *w, char *p, char *e)
{
	ulong nums[768];
	int n, nnums, i;
	char *ep;
	ulong seg[2];

	nnums = 0;
	while(p < e && *p != '\n'){
		while(p < e && (*p == ' ' || *p == '\t'))
			p++;
		if(p >= e || *p == '\n')
			break;
		if(nnums >= (int)(sizeof nums / sizeof nums[0]))
			return "too many style arguments";
		nums[nnums++] = strtoul(p, &ep, 10);
		if(ep == p)
			return "bad style syntax";
		p = ep;
	}

	if(nnums == 0)
		return "missing style index";

	n = nnums;

	if(n == 1){
		/* "style 0" — clear all style data. */
		int had = w->body.fr.nstyles > 0 || w->nstyles > 0;
		winclearstyle(w);
		if(had)
			winframesync(w);
		return nil;
	}

	if(n % 3 != 0)
		return "bad style syntax: arguments must be triples of index start length";

	for(i = 0; i < n; i += 3){
		seg[0] = nums[i];    /* index */
		seg[1] = nums[i+2];  /* length */
		winsetstyle(w, nums[i+1], seg, 1);
	}

	/* Only redraw if any segment touches the visible window. */
	{
		Text *t = &w->body;
		ulong org  = t->org;
		ulong fend = org + t->fr.nchars;
		for(i = 0; i < n; i += 3){
			ulong sstart = nums[i+1];
			ulong send   = sstart + nums[i+2];
			if(send > org && sstart < fend){
				winframesync(w);
				break;
			}
		}
	}
	return nil;
}
