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
 * loadstylefile — parse a style file and populate the global acmestyles table.
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
	Acmestyle *styles, *s;
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

	styles = emalloc(nlines * sizeof(Acmestyle));
	memset(styles, 0, nlines * sizeof(Acmestyle));

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

		s = &styles[i];

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
	if(acmestyles != nil){
		for(i = 0; i < nacmestyles; i++)
			free(acmestyles[i].name);
		free(acmestyles);
	}

	acmestyles  = styles;
	nacmestyles = i;
	return 1;
}

/*
 * buildframestyles — translate file-absolute style RLE data into a
 * frame-relative RLE covering only the characters currently visible in the
 * frame ([org, org+frnchars)).
 *
 * wstyles/nwstyles — the window's file-absolute {index,length} pairs
 * org              — file position of the first character in the frame
 * frnchars         — number of runes currently in the frame
 * out / nout       — caller receives a freshly allocated array (free after use)
 */
static void
buildframestyles(ulong *wstyles, int nwstyles, ulong org, ulong frnchars,
                 ulong **out, int *nout)
{
	ulong *res;
	int nres, i;
	ulong filepos, frpos, from, to, fr_from, fr_to;

	/* worst case: each window entry produces one segment + one gap fill */
	res = emalloc(2 * (nwstyles * 2 + 1) * sizeof(ulong));
	nres = 0;
	frpos = 0;
	filepos = 0;

	for(i = 0; i < nwstyles && filepos < org + frnchars; i++){
		ulong sidx = wstyles[i*2];
		ulong slen = wstyles[i*2 + 1];
		ulong seg_end = filepos + slen;

		/* intersect segment [filepos, seg_end) with frame [org, org+frnchars) */
		from = filepos < org ? org : filepos;
		to   = seg_end < org + frnchars ? seg_end : org + frnchars;

		if(from < to){
			fr_from = from - org;
			fr_to   = to   - org;

			/* fill any gap before this segment with style 0 */
			if(fr_from > frpos){
				res[nres*2]     = 0;
				res[nres*2 + 1] = fr_from - frpos;
				nres++;
				frpos = fr_from;
			}

			res[nres*2]     = sidx;
			res[nres*2 + 1] = fr_to - fr_from;
			nres++;
			frpos = fr_to;
		}

		filepos = seg_end;
	}

	/* Always close the frame with an explicit style-0 segment so that
	 * f->styles covers the full [0, frnchars) range.
	 *
	 * Without this, frstyleinsert's "last-segment" fallback
	 * (|| i == f->nstyles-1) fires for textfill insertions beyond the
	 * last styled span and incorrectly extends that span.  The result is
	 * visible after undo: chars pushed off the frame during each undo
	 * step trigger textfill, which keeps extending the final styled
	 * segment until it covers positions the user later types into. */
	if(frpos < frnchars){
		res[nres*2]     = 0;
		res[nres*2 + 1] = frnchars - frpos;
		nres++;
	}
	*out  = res;
	*nout = nres;
}

/*
 * winframesync — rebuild the frame's stylecols from the global style table,
 * translate w->styles from file-absolute to frame-relative coordinates, then
 * install everything into the frame and repaint.
 *
 * Call this after changing w->styles, after a frame reinit, or after
 * textsetorigin changes t->org.
 */
void
winframesync(Window *w)
{
	Frame *f;
	Text *t;
	Image **sc;
	ulong *frstyles, org, frnchars;
	int nfrstyles, i, j;

	t = &w->body;
	f = &t->fr;
	org     = t->org;
	frnchars = (ulong)f->nchars;

	if(nacmestyles == 0 || w->nstyles == 0){
		frsetstyles(f, 0, nil, 0, nil);
		return;
	}

	/*
	 * Build a flat Image* array: nacmestyles × NCOL entries.
	 * Entry 0 (the default) is special: any nil slot inherits from f->cols.
	 */
	sc = emalloc(nacmestyles * NCOL * sizeof(Image*));
	for(i = 0; i < nacmestyles; i++){
		for(j = 0; j < NCOL; j++){
			Image *img = acmestyles[i].cols[j];
			if(img == nil)
				img = f->cols[j];	/* inherit default */
			sc[i * NCOL + j] = img;
		}
	}

	/*
	 * Translate w->styles (file-absolute) into a frame-relative RLE
	 * covering only [org, org+frnchars).
	 */
	buildframestyles(w->styles, w->nstyles, org, frnchars, &frstyles, &nfrstyles);

	frsetstyles(f, nacmestyles, sc, nfrstyles, frstyles);
	free(sc);
	free(frstyles);
	frredraw(f);
	flushimage(display, 1);
}

/*
 * winsetstyle — splice new RLE style segments into w->styles.
 *
 *   start   — character offset where the new segments begin
 *   newseg  — flat array of {style_index, length} pairs
 *   nnewseg — number of pairs (total ulongs = 2*nnewseg)
 *
 * Special case: if nnewseg == 0, clear all styles.
 */
void
winsetstyle(Window *w, ulong start, ulong *newseg, int nnewseg)
{
	ulong *old, *res;
	int nold, nres, j;
	ulong pos, newlen, end;

	if(nnewseg == 0){
		/* clear everything */
		free(w->styles);
		w->styles  = nil;
		w->nstyles = 0;
		return;
	}

	old  = w->styles;
	nold = w->nstyles;

	/* compute total length covered by the new segments */
	newlen = 0;
	for(j = 0; j < nnewseg; j++)
		newlen += newseg[j*2 + 1];
	end = start + newlen;

	/*
	 * Allocate worst case: existing segments + new segments + 2 split
	 * fragments + 1 gap fill = nold + nnewseg + 3 entries.
	 */
	res  = emalloc(2 * (nold + nnewseg + 3) * sizeof(ulong));
	nres = 0;

	/*
	 * Phase 1 — copy old segments that lie entirely before 'start'.
	 * Trim any segment that straddles 'start'.
	 * 'covered' tracks the file position up to which res[] has been written.
	 */
	ulong covered = 0;
	pos = 0;
	for(j = 0; j < nold; j++){
		ulong sidx = old[j*2];
		ulong slen = old[j*2+1];
		ulong send = pos + slen;

		if(send <= start){
			/* entirely before: copy verbatim */
			res[nres*2]     = sidx;
			res[nres*2 + 1] = slen;
			nres++;
			covered = send;
		} else if(pos < start){
			/* straddles start: keep the prefix */
			res[nres*2]     = sidx;
			res[nres*2 + 1] = start - pos;
			nres++;
			covered = start;
		}
		/* segments that start at or after 'start' are handled below */
		pos += slen;
		if(pos >= start)
			break;
	}

	/*
	 * Gap fill — if the old array didn't reach 'start' (including the
	 * common case of an empty array), insert an explicit style-0 entry
	 * so the new segments land at the correct file position.
	 */
	if(covered < start){
		res[nres*2]     = 0;
		res[nres*2 + 1] = start - covered;
		nres++;
	}

	/*
	 * Phase 2 — insert the new segments.
	 */
	for(j = 0; j < nnewseg; j++){
		res[nres*2]     = newseg[j*2];
		res[nres*2 + 1] = newseg[j*2 + 1];
		nres++;
	}

	/*
	 * Phase 3 — copy old segments that lie entirely after 'end'.
	 * Trim any segment that straddles 'end'.
	 */
	pos = 0;
	for(j = 0; j < nold; j++){
		ulong sidx = old[j*2];
		ulong slen = old[j*2+1];
		ulong send = pos + slen;

		if(send > end){
			ulong from = (pos < end) ? end : pos;
			res[nres*2]     = sidx;
			res[nres*2 + 1] = send - from;
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
 * Format (p points past "style ", e is one-past-end of the message):
 *
 *   <index>                                            clear entire document
 *   <index> <start> <length>                           one segment
 *   <index> <start> <length> [<index> <start> <length> ...]  multiple segments
 *
 * Every entry carries its own absolute start position, so non-contiguous
 * ranges need no gap-filling.  winframesync is called once at the end.
 *
 * Returns an error string, or nil on success.
 */
char*
ctlstyleparse(Window *w, char *p, char *e)
{
	ulong nums[768];	/* up to 256 {index,start,length} triples */
	int n, nnums, i;
	char *ep;
	ulong seg[2];

	/* collect all numbers from the remainder of the line */
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
		/*
		 * "style 0" — clear entire document.
		 * Only redraw if the frame is currently showing styled content.
		 */
		int had = w->body.fr.nstyles > 0;
		winsetstyle(w, 0, nil, 0);
		if(had)
			winframesync(w);
		return nil;
	}

	/*
	 * "style <index> <start> <length> [<index> <start> <length> ...]"
	 * Every entry is a triple; total number count must be a multiple of 3.
	 */
	if(n % 3 != 0)
		return "bad style syntax: arguments must be triples of index start length";

	/*
	 * Apply each triple independently via winsetstyle so that
	 * non-contiguous ranges work without gap-filling.
	 * winframesync is deferred until all segments are installed.
	 */
	for(i = 0; i < n; i += 3){
		seg[0] = nums[i];	/* index */
		seg[1] = nums[i+2];	/* length */
		winsetstyle(w, nums[i+1], seg, 1);
	}

	/*
	 * Only redraw if at least one of the new segments falls within the
	 * currently visible frame window [t->org, t->org + f->nchars).
	 * Segments that are entirely above or below the scroll position have
	 * no effect on the display and need no repaint.
	 */
	{
		Text *t = &w->body;
		ulong org = t->org;
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
