#define	FOR	57346
#define	IN	57347
#define	WHILE	57348
#define	IF	57349
#define	NOT	57350
#define	TWIDDLE	57351
#define	BANG	57352
#define	SUBSHELL	57353
#define	SWITCH	57354
#define	FN	57355
#define	WORD	57356
#define	REDIR	57357
#define	REDIRW	57358
#define	DUP	57359
#define	PIPE	57360
#define	SUB	57361
#define	SIMPLE	57362
#define	ARGLIST	57363
#define	WORDS	57364
#define	BRACE	57365
#define	PAREN	57366
#define	PCMD	57367
#define	PIPEFD	57368
#define	ANDAND	57369
#define	OROR	57370
#define	COUNT	57371

#line	13	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
#include "rc.h"
#include "fns.h"

#line	16	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
typedef union {
	struct tree *tree;
} YYSTYPE;
extern	int	yyerrflag;
#ifndef	YYMAXDEPTH
#define	YYMAXDEPTH	150
#endif
YYSTYPE	yylval;
YYSTYPE	yyval;
#define YYEOFCODE 1
#define YYERRCODE 2
static	const	short	yyexca[] =
{-1, 0,
	1, 1,
	-2, 18,
-1, 1,
	1, -1,
	-2, 0,
};
#define	YYNPROD	71
#define	YYPRIVATE 57344
#define	YYLAST	374
static	const	short	yyact[] =
{
  65,   3,  63,  64,  40,   3,  41,  34,  39,  69,
  34,  91,  17,  59,  60,  61,  62,  32,  33,  68,
  32,  33,  36,  92,  30,  31,  21,  30,  31,   5,
  75,  29,  87,  77,  78,  79,  45,  45,  45,  40,
 100,  96,  90,  34,  83,  19,  45,  20,  38,  45,
  45,  45,  43,  56,  58, 101,  74,  42,  76,   2,
  80,  88,  70,  12,  35,  71,  72,  73,  45,  37,
  93,  67,   4,  86,  57,  85,   4, 102,  34, 108,
  87, 112,  98,  99,  84,  82, 103,  81,  32,  33,
  45,   1,  44,  89,  18,  45,  45,  10,  13, 106,
  66,  37,  45,   0, 107,   0,   0,   0,   0, 111,
   0,  94,  95, 113,  45,  45, 104,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
 105,   0,   0,  45,  45,  46,  47,  48,  49,  50,
  51,  52,  53,  54,  55,  25,  19,  28,  20,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,  22,  24,  23,   0,   0,   0,   0,   0,
  27,   7,  26,   8,   6,   0,  11,  14,  15,   9,
  16,  25,  19,  28,  20,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  22,  24,
  23,   0,   0,   0,  17,   0,  27,   0,  26,  46,
  47,  48,  49,  50,  51,  52,  53,  54,  55,  25,
   0,  28,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0, 110,   0,   0,   0,  22,  24,  23,   0,
   0,   0,   0,   0,  27,   0,  26,  46,  47,  48,
  49,  50,  51,  52,  53,  54,  55,  25,   0,  28,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
 109,   0,   0,   0,  22,  24,  23,   0,   0,   0,
   0,   0,  27,   0,  26,  46,  47,  48,  49,  50,
  51,  52,  53,  54,  55,  25,   0,  28,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  97,   0,
   0,   0,  22,  24,  23,   0,   0,   0,   0,   0,
  27,   0,  26,  46,  47,  48,  49,  50,  51,  52,
  53,  54,  55,  25,   0,  28,  46,  47,  48,  49,
  50,  51,  52,  53,  54,  55,  25,   0,  28,   0,
  22,  24,  23,   0,   0,   0,  17,   0,  27,   0,
  26,   0,   0,  22,  24,  23,   0,   0,   0,   0,
   0,  27,   0,  26
};
static	const	short	yypact[] =
{
 167,-1000,  -3,  -8, 167,  30,   0, -33, -35, 332,
 131, 332, 167, 167, 167, 167,-1000, 167, -21, 332,
-1000,-1000, 332, 332, 332,-1000, -25,-1000, -25,-1000,
-1000,-1000, 167, 167, 167,-1000,-1000,  30,-1000,-1000,
 167, 332,-1000,   2,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,   2,-1000,   2,  25,
  25,  25,  25, 319, -27, -11, 167,-1000, 332, 332,
   2,  22,-1000,-1000,-1000, 281,-1000,  25,  25,-1000,
-1000, 167, 167,  13,  50, 167, -25, 332, 332,-1000,
   2,-1000,-1000,-1000,   2,-1000,-1000,-1000,  60,  60,
-1000,-1000,-1000,  60,-1000,-1000, 243, 205, 167,-1000,
-1000,  60, 167,  60
};
static	const	short	yypgo[] =
{
   0,  59,  48,  29,   3,  71, 100,  98,  22,  63,
   0,  97,  94,  42,  26,  92,   2,  91,  87,  85,
  81,  79,  75,  73
};
static	const	short	yyr1[] =
{
   0,  17,  17,   1,   1,   4,   4,   5,   5,   6,
   6,   3,   2,   7,   8,   8,   9,   9,  10,  10,
  18,  10,  19,  10,  20,  10,  21,  10,  22,  10,
  23,  10,  10,  10,  10,  10,  10,  10,  10,  10,
  10,  10,  10,  11,  11,  11,  12,  12,  13,  13,
  13,  14,  14,  14,  14,  14,  14,  14,  14,  15,
  15,  15,  15,  15,  15,  15,  15,  15,  15,  16,
  16
};
static	const	short	yyr2[] =
{
   0,   0,   2,   1,   2,   1,   2,   2,   2,   1,
   2,   3,   3,   3,   0,   2,   2,   1,   0,   2,
   0,   4,   0,   4,   0,   8,   0,   6,   0,   4,
   0,   4,   1,   3,   3,   3,   3,   2,   2,   2,
   2,   3,   2,   1,   2,   2,   1,   3,   1,   1,
   3,   2,   5,   2,   2,   1,   2,   3,   2,   1,
   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,
   2
};
static	const	short	yychk[] =
{
-1000, -17,  -1, -10,  -5,  -3,   7,   4,   6,  12,
 -11,   9,  -9,  -7,  10,  11,  13,  37, -12,  15,
  17, -14,  31,  33,  32,  14,  41,  39,  16,  34,
  35,  36,  28,  29,  18,  -1,  -8,  -9,  -2,   8,
  39,  39,  -2, -13, -15, -14,   4,   5,   6,   7,
   8,   9,  10,  11,  12,  13, -13,  -9, -13, -10,
 -10, -10, -10, -16,  -4, -10,  -6,  -5,  40,  30,
 -13, -13, -13, -13,  -3, -16,  -3, -10, -10, -10,
  -8, -18, -19,  -4, -13, -22, -23,  30, -16,  -3,
 -13,  38,  34,  -4, -13, -13,  19,  27, -10, -10,
  27,   5,  27, -10,  -3, -13, -16, -16, -21,  27,
  27, -10, -20, -10
};
static	const	short	yydef[] =
{
  -2,  -2,   0,   3,  18,  14,   0,   0,   0,   0,
  32,   0,  18,  18,  18,  18,  69,  18,  43,   0,
  17,  46,   0,   0,   0,  55,   0,  69,   0,   2,
   7,   8,  18,  18,  18,   4,  19,  14,  20,  22,
  18,   0,  28,  30,  48,  49,  59,  60,  61,  62,
  63,  64,  65,  66,  67,  68,  44,  45,  69,  37,
  38,  39,  40,  42,   0,   5,  18,   9,   0,   0,
  16,  51,  53,  54,  56,   0,  58,  34,  35,  36,
  15,  18,  18,   0,   0,  18,   0,   0,  33,  41,
  70,  11,  10,   6,  13,  47,  69,  57,  21,  23,
  12,  69,  26,  29,  31,  50,   0,   0,  18,  52,
  24,  27,  18,  25
};
static	const	short	yytok1[] =
{
   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  34,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,  33,   0,  31,   0,  36,   0,
  39,  27,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  35,
   0,  40,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,  30,   0,  41,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  37,   0,  38
};
static	const	short	yytok2[] =
{
   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,
  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,
  22,  23,  24,  25,  26,  28,  29,  32
};
static	const	long	yytok3[] =
{
   0
};
#define YYFLAG 		-1000
#define YYERROR		goto yyerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#define	yyclearin	yychar = -1
#define	yyerrok		yyerrflag = 0

#ifdef	yydebug
#include	"y.debug"
#else
#define	yydebug		0
static	const	char*	yytoknames[1];		/* for debugging */
static	const	char*	yystates[1];		/* for debugging */
#endif

/*	parser for yacc output	*/
#ifdef YYARG
#define	yynerrs		yyarg->yynerrs
#define	yyerrflag	yyarg->yyerrflag
#define yyval		yyarg->yyval
#define yylval		yyarg->yylval
#else
int	yynerrs = 0;		/* number of errors */
int	yyerrflag = 0;		/* error recovery flag */
#endif

extern	int	fprint(int, char*, ...);
extern	int	sprint(char*, char*, ...);

static const char*
yytokname(int yyc)
{
	static char x[10];

	if(yyc > 0 && yyc <= sizeof(yytoknames)/sizeof(yytoknames[0]))
	if(yytoknames[yyc-1])
		return yytoknames[yyc-1];
	sprint(x, "<%d>", yyc);
	return x;
}

static const char*
yystatname(int yys)
{
	static char x[10];

	if(yys >= 0 && yys < sizeof(yystates)/sizeof(yystates[0]))
	if(yystates[yys])
		return yystates[yys];
	sprint(x, "<%d>\n", yys);
	return x;
}

static long
#ifdef YYARG
yylex1(struct Yyarg *yyarg)
#else
yylex1(void)
#endif
{
	long yychar;
	const long *t3p;
	int c;

#ifdef YYARG	
	yychar = yylex(yyarg);
#else
	yychar = yylex();
#endif
	if(yychar <= 0) {
		c = yytok1[0];
		goto out;
	}
	if(yychar < sizeof(yytok1)/sizeof(yytok1[0])) {
		c = yytok1[yychar];
		goto out;
	}
	if(yychar >= YYPRIVATE)
		if(yychar < YYPRIVATE+sizeof(yytok2)/sizeof(yytok2[0])) {
			c = yytok2[yychar-YYPRIVATE];
			goto out;
		}
	for(t3p=yytok3;; t3p+=2) {
		c = t3p[0];
		if(c == yychar) {
			c = t3p[1];
			goto out;
		}
		if(c == 0)
			break;
	}
	c = 0;

out:
	if(c == 0)
		c = yytok2[1];	/* unknown char */
	if(yydebug >= 3)
		fprint(2, "lex %.4lux %s\n", yychar, yytokname(c));
	return c;
}

int
#ifdef YYARG
yyparse(struct Yyarg *yyarg)
#else
yyparse(void)
#endif
{
	struct
	{
		YYSTYPE	yyv;
		int	yys;
	} yys[YYMAXDEPTH], *yyp, *yypt;
	const short *yyxi;
	int yyj, yym, yystate, yyn, yyg;
	long yychar;
#ifndef YYARG
	YYSTYPE save1, save2;
	int save3, save4;

	save1 = yylval;
	save2 = yyval;
	save3 = yynerrs;
	save4 = yyerrflag;
#endif

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyp = &yys[0];
	yyp--;
	goto yystack;

ret0:
	yyn = 0;
	goto ret;

ret1:
	yyn = 1;
	goto ret;

ret:
#ifndef YYARG
	yylval = save1;
	yyval = save2;
	yynerrs = save3;
	yyerrflag = save4;
#endif
	return yyn;

yystack:
	/* put a state and value onto the stack */
	if(yydebug >= 4)
		fprint(2, "char %s in %s", yytokname(yychar), yystatname(yystate));

	yyp++;
	if(yyp >= &yys[YYMAXDEPTH]) {
		yyerror("yacc stack overflow");
		goto ret1;
	}
	yyp->yys = yystate;
	yyp->yyv = yyval;

yynewstate:
	yyn = yypact[yystate];
	if(yyn <= YYFLAG)
		goto yydefault; /* simple state */
	if(yychar < 0)
#ifdef YYARG
		yychar = yylex1(yyarg);
#else
		yychar = yylex1();
#endif
	yyn += yychar;
	if(yyn < 0 || yyn >= YYLAST)
		goto yydefault;
	yyn = yyact[yyn];
	if(yychk[yyn] == yychar) { /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if(yyerrflag > 0)
			yyerrflag--;
		goto yystack;
	}

yydefault:
	/* default state action */
	yyn = yydef[yystate];
	if(yyn == -2) {
		if(yychar < 0)
#ifdef YYARG
		yychar = yylex1(yyarg);
#else
		yychar = yylex1();
#endif

		/* look through exception table */
		for(yyxi=yyexca;; yyxi+=2)
			if(yyxi[0] == -1 && yyxi[1] == yystate)
				break;
		for(yyxi += 2;; yyxi += 2) {
			yyn = yyxi[0];
			if(yyn < 0 || yyn == yychar)
				break;
		}
		yyn = yyxi[1];
		if(yyn < 0)
			goto ret0;
	}
	if(yyn == 0) {
		/* error ... attempt to resume parsing */
		switch(yyerrflag) {
		case 0:   /* brand new error */
			yyerror("syntax error");
			if(yydebug >= 1) {
				fprint(2, "%s", yystatname(yystate));
				fprint(2, "saw %s\n", yytokname(yychar));
			}
			goto yyerrlab;
		yyerrlab:
			yynerrs++;

		case 1:
		case 2: /* incompletely recovered error ... try again */
			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */
			while(yyp >= yys) {
				yyn = yypact[yyp->yys] + YYERRCODE;
				if(yyn >= 0 && yyn < YYLAST) {
					yystate = yyact[yyn];  /* simulate a shift of "error" */
					if(yychk[yystate] == YYERRCODE)
						goto yystack;
				}

				/* the current yyp has no shift onn "error", pop stack */
				if(yydebug >= 2)
					fprint(2, "error recovery pops state %d, uncovers %d\n",
						yyp->yys, (yyp-1)->yys );
				yyp--;
			}
			/* there is no state on the stack with an error shift ... abort */
			goto ret1;

		case 3:  /* no shift yet; clobber input char */
			if(yydebug >= 2)
				fprint(2, "error recovery discards %s\n", yytokname(yychar));
			if(yychar == YYEOFCODE)
				goto ret1;
			yychar = -1;
			goto yynewstate;   /* try again in the same state */
		}
	}

	/* reduction by production yyn */
	if(yydebug >= 2)
		fprint(2, "reduce %d in:\n\t%s", yyn, yystatname(yystate));

	yypt = yyp;
	yyp -= yyr2[yyn];
	yyval = (yyp+1)->yyv;
	yym = yyn;

	/* consult goto table to find next state */
	yyn = yyr1[yyn];
	yyg = yypgo[yyn];
	yyj = yyg + yyp->yys + 1;

	if(yyj >= YYLAST || yychk[yystate=yyact[yyj]] != -yyn)
		yystate = yyact[yyg];
	switch(yym) {
		
case 1:
#line	24	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{ return 1;} break;
case 2:
#line	25	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{return !compile(yypt[-1].yyv.tree);} break;
case 4:
#line	27	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree2(';', yypt[-1].yyv.tree, yypt[-0].yyv.tree);} break;
case 6:
#line	29	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree2(';', yypt[-1].yyv.tree, yypt[-0].yyv.tree);} break;
case 8:
#line	31	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree1('&', yypt[-1].yyv.tree);} break;
case 11:
#line	34	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree1(BRACE, yypt[-1].yyv.tree);} break;
case 12:
#line	35	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree1(PCMD, yypt[-1].yyv.tree);} break;
case 13:
#line	36	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree2('=', yypt[-2].yyv.tree, yypt[-0].yyv.tree);} break;
case 14:
#line	37	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=0;} break;
case 15:
#line	38	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung2(yypt[-1].yyv.tree, yypt[-1].yyv.tree->child[0], yypt[-0].yyv.tree);} break;
case 16:
#line	39	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung1(yypt[-1].yyv.tree, yypt[-1].yyv.tree->rtype==HERE?heredoc(yypt[-0].yyv.tree):yypt[-0].yyv.tree);} break;
case 18:
#line	41	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=0;} break;
case 19:
#line	42	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=epimung(yypt[-1].yyv.tree, yypt[-0].yyv.tree);} break;
case 20:
#line	43	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{skipnl();} break;
case 21:
#line	44	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung2(yypt[-3].yyv.tree, yypt[-2].yyv.tree, yypt[-0].yyv.tree);} break;
case 22:
#line	45	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{skipnl();} break;
case 23:
#line	45	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung1(yypt[-2].yyv.tree, yypt[-0].yyv.tree);} break;
case 24:
#line	46	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{skipnl();} break;
case 25:
#line	55	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung3(yypt[-7].yyv.tree, yypt[-5].yyv.tree, yypt[-3].yyv.tree ? yypt[-3].yyv.tree : tree1(PAREN, yypt[-3].yyv.tree), yypt[-0].yyv.tree);} break;
case 26:
#line	56	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{skipnl();} break;
case 27:
#line	57	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung3(yypt[-5].yyv.tree, yypt[-3].yyv.tree, (struct tree *)0, yypt[-0].yyv.tree);} break;
case 28:
#line	58	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{skipnl();} break;
case 29:
#line	59	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung2(yypt[-3].yyv.tree, yypt[-2].yyv.tree, yypt[-0].yyv.tree);} break;
case 30:
#line	60	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{skipnl();} break;
case 31:
#line	61	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree2(SWITCH, yypt[-2].yyv.tree, yypt[-0].yyv.tree);} break;
case 32:
#line	62	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=simplemung(yypt[-0].yyv.tree);} break;
case 33:
#line	63	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung2(yypt[-2].yyv.tree, yypt[-1].yyv.tree, yypt[-0].yyv.tree);} break;
case 34:
#line	64	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree2(ANDAND, yypt[-2].yyv.tree, yypt[-0].yyv.tree);} break;
case 35:
#line	65	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree2(OROR, yypt[-2].yyv.tree, yypt[-0].yyv.tree);} break;
case 36:
#line	66	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung2(yypt[-1].yyv.tree, yypt[-2].yyv.tree, yypt[-0].yyv.tree);} break;
case 37:
#line	67	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung2(yypt[-1].yyv.tree, yypt[-1].yyv.tree->child[0], yypt[-0].yyv.tree);} break;
case 38:
#line	68	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung3(yypt[-1].yyv.tree, yypt[-1].yyv.tree->child[0], yypt[-1].yyv.tree->child[1], yypt[-0].yyv.tree);} break;
case 39:
#line	69	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung1(yypt[-1].yyv.tree, yypt[-0].yyv.tree);} break;
case 40:
#line	70	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung1(yypt[-1].yyv.tree, yypt[-0].yyv.tree);} break;
case 41:
#line	71	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree2(FN, yypt[-1].yyv.tree, yypt[-0].yyv.tree);} break;
case 42:
#line	72	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree1(FN, yypt[-0].yyv.tree);} break;
case 44:
#line	74	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree2(ARGLIST, yypt[-1].yyv.tree, yypt[-0].yyv.tree);} break;
case 45:
#line	75	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree2(ARGLIST, yypt[-1].yyv.tree, yypt[-0].yyv.tree);} break;
case 47:
#line	77	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree2('^', yypt[-2].yyv.tree, yypt[-0].yyv.tree);} break;
case 48:
#line	78	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{lastword=1; yypt[-0].yyv.tree->type=WORD;} break;
case 50:
#line	80	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree2('^', yypt[-2].yyv.tree, yypt[-0].yyv.tree);} break;
case 51:
#line	81	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree1('$', yypt[-0].yyv.tree);} break;
case 52:
#line	82	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree2(SUB, yypt[-3].yyv.tree, yypt[-1].yyv.tree);} break;
case 53:
#line	83	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree1('"', yypt[-0].yyv.tree);} break;
case 54:
#line	84	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree1(COUNT, yypt[-0].yyv.tree);} break;
case 56:
#line	86	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree1('`', yypt[-0].yyv.tree);} break;
case 57:
#line	87	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree1(PAREN, yypt[-1].yyv.tree);} break;
case 58:
#line	88	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=mung1(yypt[-1].yyv.tree, yypt[-0].yyv.tree); yyval.tree->type=PIPEFD;} break;
case 69:
#line	90	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=(struct tree*)0;} break;
case 70:
#line	91	"/Users/cptaffe/.claude/jobs/edc659f2/tmp/unix/mkwasm/rc/syn.y"
{yyval.tree=tree2(WORDS, yypt[-1].yyv.tree, yypt[-0].yyv.tree);} break;
	}
	goto yystack;  /* stack new state and value */
}
