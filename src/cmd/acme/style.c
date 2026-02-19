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
 * winframesync — compose all style layers and repaint the frame.
 *
 * Each window has up to MAXSTYLAYERS independent style layers stored as
 * file-absolute {index, length} RLEs.  Layer N overrides layer N-1 at any
 * position where layer N has a non-zero style index.  Style index 0 is
 * always transparent (it does not cover a non-zero style from a lower layer).
 *
 * The composition is done with a flat per-character array over the visible
 * frame window [org, org+frnchars), which is then RLE-compressed and handed
 * to frsetstyles.
 *
 * Call after changing any layer, after a frame reinit, or after
 * textsetorigin changes t->org.
 */
void
winframesync(Window *w)
{
	Frame *f;
	Text *t;
	Image **sc;
	ulong *composed, *frstyles, org, frnchars;
	int nfrstyles, i, j, L, hasstyles;
	ulong k;

	t = &w->body;
	f = &t->fr;
	org      = t->org;
	frnchars = (ulong)f->nchars;

	/* Quick exit if no style file was loaded or no layer has data. */
	hasstyles = 0;
	for(L = 0; L < w->nlayers; L++)
		if(w->stylelayers[L] != nil){ hasstyles = 1; break; }

	if(nstyles == 0 || !hasstyles){
		frsetstyles(f, 0, nil, 0, nil);
		frredraw(f);
		flushimage(display, 1);
		return;
	}

	/*
	 * Build the Image* colour table: nstyles × NCOL entries.
	 * Nil slots inherit the frame's default colour for that column.
	 */
	sc = emalloc(nstyles * NCOL * sizeof(Image*));
	for(i = 0; i < nstyles; i++)
		for(j = 0; j < NCOL; j++){
			Image *img = styles[i].cols[j];
			if(img == nil)
				img = f->cols[j];
			sc[i * NCOL + j] = img;
		}

	/*
	 * Compose all layers into a per-character style index array.
	 * Layers are applied low-to-high so that higher layers overwrite.
	 * Style index 0 is skipped (transparent).
	 */
	composed = emalloc(frnchars * sizeof(ulong));
	memset(composed, 0, frnchars * sizeof(ulong));

	for(L = 0; L < w->nlayers; L++){
		ulong *ls  = w->stylelayers[L];
		int    nls = w->nstylelayers[L];
		ulong  filepos = 0;
		if(ls == nil) continue;
		for(i = 0; i < nls; i++){
			ulong sidx    = ls[i*2];
			ulong slen    = ls[i*2+1];
			ulong seg_end = filepos + slen;
			if(sidx != 0){
				ulong from    = filepos < org          ? org          : filepos;
				ulong to      = seg_end < org+frnchars ? seg_end      : org+frnchars;
				if(from < to){
					ulong fr_from = from - org;
					ulong fr_to   = to   - org;
					for(k = fr_from; k < fr_to; k++)
						composed[k] = sidx;
				}
			}
			filepos = seg_end;
			if(filepos >= org + frnchars)
				break;
		}
	}

	/*
	 * RLE-compress composed[] into frstyles.
	 * Worst case: every character has a different style → frnchars entries.
	 */
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
 * winclearstyle — free style data in every layer of w.
 *
 * Does not call winframesync; the caller is responsible for repainting
 * (either immediately via winframesync or implicitly via the next textredraw).
 */
void
winclearstyle(Window *w)
{
	int L;
	for(L = 0; L < w->nlayers; L++){
		free(w->stylelayers[L]);
		w->stylelayers[L]  = nil;
		w->nstylelayers[L] = 0;
	}
	/* nlayers is not reset: persistent ctl fids keep their layer indices. */
}

/*
 * winsetstyle — splice new RLE style segments into one layer of w.
 *
 *   layer   — which layer to update (0 = lowest priority)
 *   start   — character offset where the new segments begin
 *   newseg  — flat array of {style_index, length} pairs
 *   nnewseg — number of pairs (total ulongs = 2*nnewseg)
 *
 * Special case: if nnewseg == 0, clear this layer entirely.
 */
void
winsetstyle(Window *w, int layer, ulong start, ulong *newseg, int nnewseg)
{
	ulong *old, *res;
	int nold, nres, j;
	ulong pos, newlen, end;

	if(layer < 0 || layer >= MAXSTYLAYERS)
		return;

	if(nnewseg == 0){
		free(w->stylelayers[layer]);
		w->stylelayers[layer]  = nil;
		w->nstylelayers[layer] = 0;
		return;
	}

	old  = w->stylelayers[layer];
	nold = w->nstylelayers[layer];

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
	w->stylelayers[layer]  = res;
	w->nstylelayers[layer] = nres;
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
ctlstyleparse(Window *w, int layer, char *p, char *e)
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
		 * "style 0" — clear this layer.
		 * Other layers are unaffected.  Only redraw if the frame has
		 * style data that might now look different.
		 */
		int had = w->body.fr.nstyles > 0;
		winsetstyle(w, layer, 0, nil, 0);
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
		winsetstyle(w, layer, nums[i+1], seg, 1);
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
