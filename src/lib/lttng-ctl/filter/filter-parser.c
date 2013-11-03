/* A Bison parser, made by GNU Bison 2.7.12-4996.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.7.12-4996"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
/* Line 371 of yacc.c  */
#line 1 "filter-parser.y"

/*
 * filter-parser.y
 *
 * LTTng filter expression parser
 *
 * Copyright 2012 - Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License, version 2.1 only,
 * as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Grammar inspired from http://www.quut.com/c/ANSI-C-grammar-y.html
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include "filter-ast.h"
#include "filter-parser.h"

#include <common/macros.h>

LTTNG_HIDDEN
int yydebug;
LTTNG_HIDDEN
int filter_parser_debug = 0;

LTTNG_HIDDEN
int yyparse(struct filter_parser_ctx *parser_ctx, yyscan_t scanner);
LTTNG_HIDDEN
int yylex(union YYSTYPE *yyval, yyscan_t scanner);
LTTNG_HIDDEN
int yylex_init_extra(struct filter_parser_ctx *parser_ctx, yyscan_t * ptr_yy_globals);
LTTNG_HIDDEN
int yylex_destroy(yyscan_t yyparser_ctx);
LTTNG_HIDDEN
void yyrestart(FILE * in_str, yyscan_t parser_ctx);

struct gc_string {
	struct cds_list_head gc;
	size_t alloclen;
	char s[];
};

static const char *node_type_to_str[] = {
	[ NODE_UNKNOWN ] = "NODE_UNKNOWN",
	[ NODE_ROOT ] = "NODE_ROOT",
	[ NODE_EXPRESSION ] = "NODE_EXPRESSION",
	[ NODE_OP ] = "NODE_OP",
	[ NODE_UNARY_OP ] = "NODE_UNARY_OP",
};

LTTNG_HIDDEN
const char *node_type(struct filter_node *node)
{
	if (node->type < NR_NODE_TYPES)
		return node_type_to_str[node->type];
	else
		return NULL;
}

static struct gc_string *gc_string_alloc(struct filter_parser_ctx *parser_ctx,
					 size_t len)
{
	struct gc_string *gstr;
	size_t alloclen;

	/* TODO: could be faster with find first bit or glib Gstring */
	/* sizeof long to account for malloc header (int or long ?) */
	for (alloclen = 8; alloclen < sizeof(long) + sizeof(*gstr) + len;
	     alloclen *= 2);

	gstr = malloc(alloclen);
	cds_list_add(&gstr->gc, &parser_ctx->allocated_strings);
	gstr->alloclen = alloclen;
	return gstr;
}

/*
 * note: never use gc_string_append on a string that has external references.
 * gsrc will be garbage collected immediately, and gstr might be.
 * Should only be used to append characters to a string literal or constant.
 */
LTTNG_HIDDEN
struct gc_string *gc_string_append(struct filter_parser_ctx *parser_ctx,
				   struct gc_string *gstr,
				   struct gc_string *gsrc)
{
	size_t newlen = strlen(gsrc->s) + strlen(gstr->s) + 1;
	size_t alloclen;

	/* TODO: could be faster with find first bit or glib Gstring */
	/* sizeof long to account for malloc header (int or long ?) */
	for (alloclen = 8; alloclen < sizeof(long) + sizeof(*gstr) + newlen;
	     alloclen *= 2);

	if (alloclen > gstr->alloclen) {
		struct gc_string *newgstr;

		newgstr = gc_string_alloc(parser_ctx, newlen);
		strcpy(newgstr->s, gstr->s);
		strcat(newgstr->s, gsrc->s);
		cds_list_del(&gstr->gc);
		free(gstr);
		gstr = newgstr;
	} else {
		strcat(gstr->s, gsrc->s);
	}
	cds_list_del(&gsrc->gc);
	free(gsrc);
	return gstr;
}

LTTNG_HIDDEN
void setstring(struct filter_parser_ctx *parser_ctx, YYSTYPE *lvalp, const char *src)
{
	lvalp->gs = gc_string_alloc(parser_ctx, strlen(src) + 1);
	strcpy(lvalp->gs->s, src);
}

static struct filter_node *make_node(struct filter_parser_ctx *scanner,
				  enum node_type type)
{
	struct filter_ast *ast = filter_parser_get_ast(scanner);
	struct filter_node *node;

	node = malloc(sizeof(*node));
	if (!node)
		return NULL;
	memset(node, 0, sizeof(*node));
	node->type = type;
	cds_list_add(&node->gc, &ast->allocated_nodes);

	switch (type) {
	case NODE_ROOT:
		fprintf(stderr, "[error] %s: trying to create root node\n", __func__);
		break;

	case NODE_EXPRESSION:
		break;
	case NODE_OP:
		break;
	case NODE_UNARY_OP:
		break;

	case NODE_UNKNOWN:
	default:
		fprintf(stderr, "[error] %s: unknown node type %d\n", __func__,
			(int) type);
		break;
	}

	return node;
}

static struct filter_node *make_op_node(struct filter_parser_ctx *scanner,
			enum op_type type,
			struct filter_node *lchild,
			struct filter_node *rchild)
{
	struct filter_ast *ast = filter_parser_get_ast(scanner);
	struct filter_node *node;

	node = malloc(sizeof(*node));
	if (!node)
		return NULL;
	memset(node, 0, sizeof(*node));
	node->type = NODE_OP;
	cds_list_add(&node->gc, &ast->allocated_nodes);
	node->u.op.type = type;
	node->u.op.lchild = lchild;
	node->u.op.rchild = rchild;
	return node;
}

LTTNG_HIDDEN
void yyerror(struct filter_parser_ctx *parser_ctx, yyscan_t scanner, const char *str)
{
	fprintf(stderr, "error %s\n", str);
}
 
LTTNG_HIDDEN
int yywrap(void)
{
	return 1;
} 

#define parse_error(parser_ctx, str)				\
do {								\
	yyerror(parser_ctx, parser_ctx->scanner, YY_("parse error: " str "\n"));	\
	YYERROR;						\
} while (0)

static void free_strings(struct cds_list_head *list)
{
	struct gc_string *gstr, *tmp;

	cds_list_for_each_entry_safe(gstr, tmp, list, gc)
		free(gstr);
}

static struct filter_ast *filter_ast_alloc(void)
{
	struct filter_ast *ast;

	ast = malloc(sizeof(*ast));
	if (!ast)
		return NULL;
	memset(ast, 0, sizeof(*ast));
	CDS_INIT_LIST_HEAD(&ast->allocated_nodes);
	ast->root.type = NODE_ROOT;
	return ast;
}

static void filter_ast_free(struct filter_ast *ast)
{
	struct filter_node *node, *tmp;

	cds_list_for_each_entry_safe(node, tmp, &ast->allocated_nodes, gc)
		free(node);
	free(ast);
}

LTTNG_HIDDEN
int filter_parser_ctx_append_ast(struct filter_parser_ctx *parser_ctx)
{
	return yyparse(parser_ctx, parser_ctx->scanner);
}

LTTNG_HIDDEN
struct filter_parser_ctx *filter_parser_ctx_alloc(FILE *input)
{
	struct filter_parser_ctx *parser_ctx;
	int ret;

	yydebug = filter_parser_debug;

	parser_ctx = malloc(sizeof(*parser_ctx));
	if (!parser_ctx)
		return NULL;
	memset(parser_ctx, 0, sizeof(*parser_ctx));

	ret = yylex_init_extra(parser_ctx, &parser_ctx->scanner);
	if (ret) {
		fprintf(stderr, "yylex_init error\n");
		goto cleanup_parser_ctx;
	}
	/* Start processing new stream */
	yyrestart(input, parser_ctx->scanner);

	parser_ctx->ast = filter_ast_alloc();
	if (!parser_ctx->ast)
		goto cleanup_lexer;
	CDS_INIT_LIST_HEAD(&parser_ctx->allocated_strings);

	if (yydebug)
		fprintf(stdout, "parser_ctx input is a%s.\n",
			isatty(fileno(input)) ? "n interactive tty" :
						" noninteractive file");

	return parser_ctx;

cleanup_lexer:
	ret = yylex_destroy(parser_ctx->scanner);
	if (!ret)
		fprintf(stderr, "yylex_destroy error\n");
cleanup_parser_ctx:
	free(parser_ctx);
	return NULL;
}

LTTNG_HIDDEN
void filter_parser_ctx_free(struct filter_parser_ctx *parser_ctx)
{
	int ret;

	free_strings(&parser_ctx->allocated_strings);
	filter_ast_free(parser_ctx->ast);
	ret = yylex_destroy(parser_ctx->scanner);
	if (ret)
		fprintf(stderr, "yylex_destroy error\n");
	free(parser_ctx);
}


/* Line 371 of yacc.c  */
#line 368 "filter-parser.c"

# ifndef YY_NULL
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULL nullptr
#  else
#   define YY_NULL 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     CHARACTER_CONSTANT_START = 258,
     SQUOTE = 259,
     STRING_LITERAL_START = 260,
     DQUOTE = 261,
     ESCSEQ = 262,
     CHAR_STRING_TOKEN = 263,
     DECIMAL_CONSTANT = 264,
     OCTAL_CONSTANT = 265,
     HEXADECIMAL_CONSTANT = 266,
     FLOAT_CONSTANT = 267,
     LSBRAC = 268,
     RSBRAC = 269,
     LPAREN = 270,
     RPAREN = 271,
     LBRAC = 272,
     RBRAC = 273,
     RARROW = 274,
     STAR = 275,
     PLUS = 276,
     MINUS = 277,
     MOD_OP = 278,
     DIV_OP = 279,
     RIGHT_OP = 280,
     LEFT_OP = 281,
     EQ_OP = 282,
     NE_OP = 283,
     LE_OP = 284,
     GE_OP = 285,
     LT_OP = 286,
     GT_OP = 287,
     AND_OP = 288,
     OR_OP = 289,
     NOT_OP = 290,
     ASSIGN = 291,
     COLON = 292,
     SEMICOLON = 293,
     DOTDOTDOT = 294,
     DOT = 295,
     EQUAL = 296,
     COMMA = 297,
     XOR_BIN = 298,
     AND_BIN = 299,
     OR_BIN = 300,
     NOT_BIN = 301,
     IDENTIFIER = 302,
     GLOBAL_IDENTIFIER = 303,
     ERROR = 304
   };
#endif
/* Tokens.  */
#define CHARACTER_CONSTANT_START 258
#define SQUOTE 259
#define STRING_LITERAL_START 260
#define DQUOTE 261
#define ESCSEQ 262
#define CHAR_STRING_TOKEN 263
#define DECIMAL_CONSTANT 264
#define OCTAL_CONSTANT 265
#define HEXADECIMAL_CONSTANT 266
#define FLOAT_CONSTANT 267
#define LSBRAC 268
#define RSBRAC 269
#define LPAREN 270
#define RPAREN 271
#define LBRAC 272
#define RBRAC 273
#define RARROW 274
#define STAR 275
#define PLUS 276
#define MINUS 277
#define MOD_OP 278
#define DIV_OP 279
#define RIGHT_OP 280
#define LEFT_OP 281
#define EQ_OP 282
#define NE_OP 283
#define LE_OP 284
#define GE_OP 285
#define LT_OP 286
#define GT_OP 287
#define AND_OP 288
#define OR_OP 289
#define NOT_OP 290
#define ASSIGN 291
#define COLON 292
#define SEMICOLON 293
#define DOTDOTDOT 294
#define DOT 295
#define EQUAL 296
#define COMMA 297
#define XOR_BIN 298
#define AND_BIN 299
#define OR_BIN 300
#define NOT_BIN 301
#define IDENTIFIER 302
#define GLOBAL_IDENTIFIER 303
#define ERROR 304



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 387 of yacc.c  */
#line 320 "filter-parser.y"

	long long ll;
	char c;
	struct gc_string *gs;
	struct filter_node *n;


/* Line 387 of yacc.c  */
#line 517 "filter-parser.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (struct filter_parser_ctx *parser_ctx, yyscan_t scanner);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

/* Line 390 of yacc.c  */
#line 544 "filter-parser.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if (! defined __GNUC__ || __GNUC__ < 2 \
      || (__GNUC__ == 2 && __GNUC_MINOR__ < 5))
#  define __attribute__(Spec) /* empty */
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif


/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(N) (N)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  61
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   71

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  50
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  21
/* YYNRULES -- Number of rules.  */
#define YYNRULES  58
/* YYNRULES -- Number of states.  */
#define YYNSTATES  87

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   304

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     5,     8,    10,    12,    14,    17,    19,
      21,    23,    25,    27,    29,    31,    33,    36,    40,    44,
      48,    50,    54,    58,    60,    63,    65,    67,    69,    71,
      73,    77,    81,    85,    87,    91,    95,    97,   101,   105,
     107,   111,   115,   119,   123,   125,   129,   133,   135,   139,
     141,   145,   147,   151,   153,   157,   159,   163,   165
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      70,     0,    -1,    52,    -1,    51,    52,    -1,     8,    -1,
       7,    -1,    54,    -1,    53,    54,    -1,     8,    -1,     7,
      -1,    47,    -1,    48,    -1,     9,    -1,    10,    -1,    11,
      -1,    12,    -1,     5,     6,    -1,     5,    53,     6,    -1,
       3,    51,     4,    -1,    15,    69,    16,    -1,    55,    -1,
      56,    40,    47,    -1,    56,    19,    47,    -1,    56,    -1,
      58,    57,    -1,    21,    -1,    22,    -1,    35,    -1,    46,
      -1,    57,    -1,    59,    20,    57,    -1,    59,    24,    57,
      -1,    59,    23,    57,    -1,    59,    -1,    60,    21,    59,
      -1,    60,    22,    59,    -1,    60,    -1,    61,    26,    60,
      -1,    61,    25,    60,    -1,    61,    -1,    62,    31,    61,
      -1,    62,    32,    61,    -1,    62,    29,    61,    -1,    62,
      30,    61,    -1,    62,    -1,    63,    27,    62,    -1,    63,
      28,    62,    -1,    63,    -1,    64,    44,    63,    -1,    64,
      -1,    65,    43,    64,    -1,    65,    -1,    66,    45,    65,
      -1,    66,    -1,    67,    33,    66,    -1,    67,    -1,    68,
      34,    67,    -1,    68,    -1,    69,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   351,   351,   353,   358,   360,   369,   371,   376,   378,
     385,   391,   398,   405,   412,   419,   426,   432,   438,   444,
     453,   455,   463,   474,   476,   484,   489,   494,   499,   507,
     509,   513,   517,   524,   526,   530,   537,   539,   543,   550,
     552,   556,   560,   564,   571,   573,   577,   584,   586,   593,
     595,   602,   604,   611,   613,   620,   622,   629,   634
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CHARACTER_CONSTANT_START", "SQUOTE",
  "STRING_LITERAL_START", "DQUOTE", "ESCSEQ", "CHAR_STRING_TOKEN",
  "DECIMAL_CONSTANT", "OCTAL_CONSTANT", "HEXADECIMAL_CONSTANT",
  "FLOAT_CONSTANT", "LSBRAC", "RSBRAC", "LPAREN", "RPAREN", "LBRAC",
  "RBRAC", "RARROW", "STAR", "PLUS", "MINUS", "MOD_OP", "DIV_OP",
  "RIGHT_OP", "LEFT_OP", "EQ_OP", "NE_OP", "LE_OP", "GE_OP", "LT_OP",
  "GT_OP", "AND_OP", "OR_OP", "NOT_OP", "ASSIGN", "COLON", "SEMICOLON",
  "DOTDOTDOT", "DOT", "EQUAL", "COMMA", "XOR_BIN", "AND_BIN", "OR_BIN",
  "NOT_BIN", "IDENTIFIER", "GLOBAL_IDENTIFIER", "ERROR", "$accept",
  "c_char_sequence", "c_char", "s_char_sequence", "s_char",
  "primary_expression", "postfix_expression", "unary_expression",
  "unary_operator", "multiplicative_expression", "additive_expression",
  "shift_expression", "relational_expression", "equality_expression",
  "and_expression", "exclusive_or_expression", "inclusive_or_expression",
  "logical_and_expression", "logical_or_expression", "expression",
  "translation_unit", YY_NULL
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    50,    51,    51,    52,    52,    53,    53,    54,    54,
      55,    55,    55,    55,    55,    55,    55,    55,    55,    55,
      56,    56,    56,    57,    57,    58,    58,    58,    58,    59,
      59,    59,    59,    60,    60,    60,    61,    61,    61,    62,
      62,    62,    62,    62,    63,    63,    63,    64,    64,    65,
      65,    66,    66,    67,    67,    68,    68,    69,    70
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     1,     1,     1,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     3,     3,     3,
       1,     3,     3,     1,     2,     1,     1,     1,     1,     1,
       3,     3,     3,     1,     3,     3,     1,     3,     3,     1,
       3,     3,     3,     3,     1,     3,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,    12,    13,    14,    15,     0,    25,    26,
      27,    28,    10,    11,    20,    23,    29,     0,    33,    36,
      39,    44,    47,    49,    51,    53,    55,    57,    58,     0,
       5,     4,     0,     2,    16,     9,     8,     0,     6,     0,
       0,     0,    24,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     1,    18,     3,    17,     7,    19,    22,    21,    30,
      32,    31,    34,    35,    38,    37,    42,    43,    40,    41,
      45,    46,    48,    50,    52,    54,    56
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    32,    33,    37,    38,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -42
static const yytype_int8 yypact[] =
{
      -3,     8,    16,   -42,   -42,   -42,   -42,    -3,   -42,   -42,
     -42,   -42,   -42,   -42,   -42,   -15,   -42,    -3,   -10,     9,
      24,     4,    14,   -41,   -38,   -34,    -7,    19,   -42,    58,
     -42,   -42,    13,   -42,   -42,   -42,   -42,    40,   -42,    43,
      15,    17,   -42,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
      -3,   -42,   -42,   -42,   -42,   -42,   -42,   -42,   -42,   -42,
     -42,   -42,   -10,   -10,     9,     9,    24,    24,    24,    24,
       4,     4,    14,   -41,   -38,   -34,    -7
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -42,   -42,    28,   -42,    26,   -42,   -42,   -16,   -42,     5,
       6,   -13,     2,    10,    11,     3,    12,     7,   -42,    62,
     -42
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
       1,    42,     2,    56,    40,    57,     3,     4,     5,     6,
      43,    58,     7,    44,    45,    30,    31,    62,     8,     9,
      30,    31,    34,    35,    36,    41,    59,    69,    70,    71,
      46,    47,    10,    50,    51,    52,    53,    76,    77,    78,
      79,    54,    55,    11,    12,    13,    64,    35,    36,    48,
      49,    72,    73,    60,    74,    75,    80,    81,    61,    66,
      63,    84,    67,    65,    68,     0,    82,    86,    83,    39,
       0,    85
};

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-42)))

#define yytable_value_is_error(Yytable_value) \
  YYID (0)

static const yytype_int8 yycheck[] =
{
       3,    17,     5,    44,    19,    43,     9,    10,    11,    12,
      20,    45,    15,    23,    24,     7,     8,     4,    21,    22,
       7,     8,     6,     7,     8,    40,    33,    43,    44,    45,
      21,    22,    35,    29,    30,    31,    32,    50,    51,    52,
      53,    27,    28,    46,    47,    48,     6,     7,     8,    25,
      26,    46,    47,    34,    48,    49,    54,    55,     0,    16,
      32,    58,    47,    37,    47,    -1,    56,    60,    57,     7,
      -1,    59
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     5,     9,    10,    11,    12,    15,    21,    22,
      35,    46,    47,    48,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
       7,     8,    51,    52,     6,     7,     8,    53,    54,    69,
      19,    40,    57,    20,    23,    24,    21,    22,    25,    26,
      29,    30,    31,    32,    27,    28,    44,    43,    45,    33,
      34,     0,     4,    52,     6,    54,    16,    47,    47,    57,
      57,    57,    59,    59,    60,    60,    61,    61,    61,    61,
      62,    62,    63,    64,    65,    66,    67
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (parser_ctx, scanner, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))

/* Error token number */
#define YYTERROR	1
#define YYERRCODE	256


/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */
#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, scanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, parser_ctx, scanner); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, struct filter_parser_ctx *parser_ctx, yyscan_t scanner)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, parser_ctx, scanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    struct filter_parser_ctx *parser_ctx;
    yyscan_t scanner;
#endif
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
  YYUSE (parser_ctx);
  YYUSE (scanner);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, struct filter_parser_ctx *parser_ctx, yyscan_t scanner)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, parser_ctx, scanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    struct filter_parser_ctx *parser_ctx;
    yyscan_t scanner;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, parser_ctx, scanner);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, struct filter_parser_ctx *parser_ctx, yyscan_t scanner)
#else
static void
yy_reduce_print (yyvsp, yyrule, parser_ctx, scanner)
    YYSTYPE *yyvsp;
    int yyrule;
    struct filter_parser_ctx *parser_ctx;
    yyscan_t scanner;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , parser_ctx, scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, parser_ctx, scanner); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULL, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULL;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULL, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, struct filter_parser_ctx *parser_ctx, yyscan_t scanner)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, parser_ctx, scanner)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    struct filter_parser_ctx *parser_ctx;
    yyscan_t scanner;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (parser_ctx);
  YYUSE (scanner);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YYUSE (yytype);
}




/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (struct filter_parser_ctx *parser_ctx, yyscan_t scanner)
#else
int
yyparse (parser_ctx, scanner)
    struct filter_parser_ctx *parser_ctx;
    yyscan_t scanner;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;


#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
static YYSTYPE yyval_default;
# define YY_INITIAL_VALUE(Value) = Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval YY_INITIAL_VALUE(yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
/* Line 1787 of yacc.c  */
#line 352 "filter-parser.y"
    {	(yyval.gs) = (yyvsp[(1) - (1)].gs);					}
    break;

  case 3:
/* Line 1787 of yacc.c  */
#line 354 "filter-parser.y"
    {	(yyval.gs) = gc_string_append(parser_ctx, (yyvsp[(1) - (2)].gs), (yyvsp[(2) - (2)].gs));		}
    break;

  case 4:
/* Line 1787 of yacc.c  */
#line 359 "filter-parser.y"
    {	(yyval.gs) = yylval.gs;					}
    break;

  case 5:
/* Line 1787 of yacc.c  */
#line 361 "filter-parser.y"
    {
			parse_error(parser_ctx, "escape sequences not supported yet");
		}
    break;

  case 6:
/* Line 1787 of yacc.c  */
#line 370 "filter-parser.y"
    {	(yyval.gs) = (yyvsp[(1) - (1)].gs);					}
    break;

  case 7:
/* Line 1787 of yacc.c  */
#line 372 "filter-parser.y"
    {	(yyval.gs) = gc_string_append(parser_ctx, (yyvsp[(1) - (2)].gs), (yyvsp[(2) - (2)].gs));		}
    break;

  case 8:
/* Line 1787 of yacc.c  */
#line 377 "filter-parser.y"
    {	(yyval.gs) = yylval.gs;					}
    break;

  case 9:
/* Line 1787 of yacc.c  */
#line 379 "filter-parser.y"
    {
			parse_error(parser_ctx, "escape sequences not supported yet");
		}
    break;

  case 10:
/* Line 1787 of yacc.c  */
#line 386 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_EXPRESSION);
			(yyval.n)->u.expression.type = AST_EXP_IDENTIFIER;
			(yyval.n)->u.expression.u.identifier = yylval.gs->s;
		}
    break;

  case 11:
/* Line 1787 of yacc.c  */
#line 392 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_EXPRESSION);
			(yyval.n)->u.expression.type = AST_EXP_GLOBAL_IDENTIFIER;
			(yyval.n)->u.expression.u.identifier = yylval.gs->s;
		}
    break;

  case 12:
/* Line 1787 of yacc.c  */
#line 399 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_EXPRESSION);
			(yyval.n)->u.expression.type = AST_EXP_CONSTANT;
			sscanf(yylval.gs->s, "%" PRIu64,
			       &(yyval.n)->u.expression.u.constant);
		}
    break;

  case 13:
/* Line 1787 of yacc.c  */
#line 406 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_EXPRESSION);
			(yyval.n)->u.expression.type = AST_EXP_CONSTANT;
			sscanf(yylval.gs->s, "0%" PRIo64,
			       &(yyval.n)->u.expression.u.constant);
		}
    break;

  case 14:
/* Line 1787 of yacc.c  */
#line 413 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_EXPRESSION);
			(yyval.n)->u.expression.type = AST_EXP_CONSTANT;
			sscanf(yylval.gs->s, "0x%" PRIx64,
			       &(yyval.n)->u.expression.u.constant);
		}
    break;

  case 15:
/* Line 1787 of yacc.c  */
#line 420 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_EXPRESSION);
			(yyval.n)->u.expression.type = AST_EXP_FLOAT_CONSTANT;
			sscanf(yylval.gs->s, "%lg",
			       &(yyval.n)->u.expression.u.float_constant);
		}
    break;

  case 16:
/* Line 1787 of yacc.c  */
#line 427 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_EXPRESSION);
			(yyval.n)->u.expression.type = AST_EXP_STRING;
			(yyval.n)->u.expression.u.string = "";
		}
    break;

  case 17:
/* Line 1787 of yacc.c  */
#line 433 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_EXPRESSION);
			(yyval.n)->u.expression.type = AST_EXP_STRING;
			(yyval.n)->u.expression.u.string = (yyvsp[(2) - (3)].gs)->s;
		}
    break;

  case 18:
/* Line 1787 of yacc.c  */
#line 439 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_EXPRESSION);
			(yyval.n)->u.expression.type = AST_EXP_STRING;
			(yyval.n)->u.expression.u.string = (yyvsp[(2) - (3)].gs)->s;
		}
    break;

  case 19:
/* Line 1787 of yacc.c  */
#line 445 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_EXPRESSION);
			(yyval.n)->u.expression.type = AST_EXP_NESTED;
			(yyval.n)->u.expression.u.child = (yyvsp[(2) - (3)].n);
		}
    break;

  case 20:
/* Line 1787 of yacc.c  */
#line 454 "filter-parser.y"
    {	(yyval.n) = (yyvsp[(1) - (1)].n);					}
    break;

  case 21:
/* Line 1787 of yacc.c  */
#line 456 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_EXPRESSION);
			(yyval.n)->u.expression.type = AST_EXP_IDENTIFIER;
			(yyval.n)->u.expression.post_op = AST_LINK_DOT;
			(yyval.n)->u.expression.u.identifier = (yyvsp[(3) - (3)].gs)->s;
			(yyval.n)->u.expression.prev = (yyvsp[(1) - (3)].n);
		}
    break;

  case 22:
/* Line 1787 of yacc.c  */
#line 464 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_EXPRESSION);
			(yyval.n)->u.expression.type = AST_EXP_IDENTIFIER;
			(yyval.n)->u.expression.post_op = AST_LINK_RARROW;
			(yyval.n)->u.expression.u.identifier = (yyvsp[(3) - (3)].gs)->s;
			(yyval.n)->u.expression.prev = (yyvsp[(1) - (3)].n);
		}
    break;

  case 23:
/* Line 1787 of yacc.c  */
#line 475 "filter-parser.y"
    {	(yyval.n) = (yyvsp[(1) - (1)].n);					}
    break;

  case 24:
/* Line 1787 of yacc.c  */
#line 477 "filter-parser.y"
    {
			(yyval.n) = (yyvsp[(1) - (2)].n);
			(yyval.n)->u.unary_op.child = (yyvsp[(2) - (2)].n);
		}
    break;

  case 25:
/* Line 1787 of yacc.c  */
#line 485 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_UNARY_OP);
			(yyval.n)->u.unary_op.type = AST_UNARY_PLUS;
		}
    break;

  case 26:
/* Line 1787 of yacc.c  */
#line 490 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_UNARY_OP);
			(yyval.n)->u.unary_op.type = AST_UNARY_MINUS;
		}
    break;

  case 27:
/* Line 1787 of yacc.c  */
#line 495 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_UNARY_OP);
			(yyval.n)->u.unary_op.type = AST_UNARY_NOT;
		}
    break;

  case 28:
/* Line 1787 of yacc.c  */
#line 500 "filter-parser.y"
    {
			(yyval.n) = make_node(parser_ctx, NODE_UNARY_OP);
			(yyval.n)->u.unary_op.type = AST_UNARY_BIN_NOT;
		}
    break;

  case 29:
/* Line 1787 of yacc.c  */
#line 508 "filter-parser.y"
    {	(yyval.n) = (yyvsp[(1) - (1)].n);					}
    break;

  case 30:
/* Line 1787 of yacc.c  */
#line 510 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_MUL, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 31:
/* Line 1787 of yacc.c  */
#line 514 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_DIV, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 32:
/* Line 1787 of yacc.c  */
#line 518 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_MOD, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 33:
/* Line 1787 of yacc.c  */
#line 525 "filter-parser.y"
    {	(yyval.n) = (yyvsp[(1) - (1)].n);					}
    break;

  case 34:
/* Line 1787 of yacc.c  */
#line 527 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_PLUS, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 35:
/* Line 1787 of yacc.c  */
#line 531 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_MINUS, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 36:
/* Line 1787 of yacc.c  */
#line 538 "filter-parser.y"
    {	(yyval.n) = (yyvsp[(1) - (1)].n);					}
    break;

  case 37:
/* Line 1787 of yacc.c  */
#line 540 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_LSHIFT, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 38:
/* Line 1787 of yacc.c  */
#line 544 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_RSHIFT, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 39:
/* Line 1787 of yacc.c  */
#line 551 "filter-parser.y"
    {	(yyval.n) = (yyvsp[(1) - (1)].n);					}
    break;

  case 40:
/* Line 1787 of yacc.c  */
#line 553 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_LT, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 41:
/* Line 1787 of yacc.c  */
#line 557 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_GT, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 42:
/* Line 1787 of yacc.c  */
#line 561 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_LE, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 43:
/* Line 1787 of yacc.c  */
#line 565 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_GE, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 44:
/* Line 1787 of yacc.c  */
#line 572 "filter-parser.y"
    {	(yyval.n) = (yyvsp[(1) - (1)].n);					}
    break;

  case 45:
/* Line 1787 of yacc.c  */
#line 574 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_EQ, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 46:
/* Line 1787 of yacc.c  */
#line 578 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_NE, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 47:
/* Line 1787 of yacc.c  */
#line 585 "filter-parser.y"
    {	(yyval.n) = (yyvsp[(1) - (1)].n);					}
    break;

  case 48:
/* Line 1787 of yacc.c  */
#line 587 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_BIN_AND, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 49:
/* Line 1787 of yacc.c  */
#line 594 "filter-parser.y"
    {	(yyval.n) = (yyvsp[(1) - (1)].n);					}
    break;

  case 50:
/* Line 1787 of yacc.c  */
#line 596 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_BIN_XOR, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 51:
/* Line 1787 of yacc.c  */
#line 603 "filter-parser.y"
    {	(yyval.n) = (yyvsp[(1) - (1)].n);					}
    break;

  case 52:
/* Line 1787 of yacc.c  */
#line 605 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_BIN_OR, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 53:
/* Line 1787 of yacc.c  */
#line 612 "filter-parser.y"
    {	(yyval.n) = (yyvsp[(1) - (1)].n);					}
    break;

  case 54:
/* Line 1787 of yacc.c  */
#line 614 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_AND, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 55:
/* Line 1787 of yacc.c  */
#line 621 "filter-parser.y"
    {	(yyval.n) = (yyvsp[(1) - (1)].n);					}
    break;

  case 56:
/* Line 1787 of yacc.c  */
#line 623 "filter-parser.y"
    {
			(yyval.n) = make_op_node(parser_ctx, AST_OP_OR, (yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
		}
    break;

  case 57:
/* Line 1787 of yacc.c  */
#line 630 "filter-parser.y"
    {	(yyval.n) = (yyvsp[(1) - (1)].n);					}
    break;

  case 58:
/* Line 1787 of yacc.c  */
#line 635 "filter-parser.y"
    {
			parser_ctx->ast->root.u.root.child = (yyvsp[(1) - (1)].n);
		}
    break;


/* Line 1787 of yacc.c  */
#line 2301 "filter-parser.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (parser_ctx, scanner, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (parser_ctx, scanner, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, parser_ctx, scanner);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, parser_ctx, scanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (parser_ctx, scanner, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, parser_ctx, scanner);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, parser_ctx, scanner);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


