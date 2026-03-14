/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */

int comment_level = 0;

%}

%option noyywrap

/*
 * Define names for regular expressions here.
 */
BOOLE (([f][aA][lL][sS][eE])|([t][rR][uU][eE]))

DIGIT [0-9]

TYPEID [A-Z_][a-zA-Z_0-9]*

OBJECTID [a-z_][a-zA-Z_0-9]*

STRING \"([^"\\]*(\\.)?(\\\n)?)*\"

SINGLE_LINE_COMMENT "--".*

%x COMMENT

%%

[ \t\r\v\f]+ {}

[\n] {
  curr_lineno++;
}

{SINGLE_LINE_COMMENT} {}

 /*
  *  Nested comments
  */

"(*" {
  comment_level = 1;
  BEGIN(COMMENT);
}

<COMMENT>{
  "(*" { comment_level++; }
  "*)" { comment_level--; if (comment_level == 0) BEGIN(INITIAL); }
  [\n] { curr_lineno++; }
  . {}
  <<EOF>> {
    cool_yylval.error_msg = "EOF in comment";
    BEGIN(INITIAL);
    return ERROR;
  }
}

"*)" {
  cool_yylval.error_msg = "Unmatched *)";
  return ERROR;
}

 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */

(?i:class)     { return CLASS; }
(?i:else)      { return ELSE; }
(?i:fi)        { return FI; }
(?i:if)        { return IF; }
(?i:in)        { return IN; }
(?i:inherits)  { return INHERITS; }
(?i:let)       { return LET; }
(?i:loop)      { return LOOP; }
(?i:pool)      { return POOL; }
(?i:then)      { return THEN; }
(?i:while)     { return WHILE; }
(?i:case)      { return CASE; }
(?i:esac)      { return ESAC; }
(?i:of)        { return OF; }
(?i:new)       { return NEW; }
(?i:not)       { return NOT; }
(?i:isvoid)    { return ISVOID; }


{DIGIT}+ {
  cool_yylval.symbol = inttable.add_string(yytext);
  return INT_CONST;
}

{BOOLE} {
  if (yytext[0] == 't') {
    cool_yylval.boolean = true;
  } else {
    cool_yylval.boolean = false;
  }

  return BOOL_CONST;
}

{TYPEID} {
  cool_yylval.symbol = stringtable.add_string(yytext);
  return TYPEID;
}

{OBJECTID} {
  cool_yylval.symbol = stringtable.add_string(yytext);
  return OBJECTID;
}


 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */

{STRING} {
  char *start = yytext+1;
  char *end = yytext + yyleng - 1; 
  
  if ( (yyleng - 2) > MAX_STR_CONST - 1 ) {
    cool_yylval.error_msg = "String constant too long";
    return ERROR;
  }
	
  string_buf_ptr = string_buf;  

  while (start < end) {
    if (*start == '\\') {
      start++;
      switch (*start) {
        case 'n':
	  *string_buf_ptr++ = '\n';
	  break;
        case 't':
	  *string_buf_ptr++ = '\t';
	  break;
	case 'b':
	  *string_buf_ptr++ = '\b';
	  break;
	case 'f':
	  *string_buf_ptr++ = '\f';
	  break;
        case '0':
	  *string_buf_ptr++ = '0';
	  break;
        case '\0':
	  cool_yylval.error_msg = "String contains null character";
	  return ERROR;
	  break;
	case '\n':
	  *string_buf_ptr++ = '\n';
	  curr_lineno++;
	  break;
        default:
	  *string_buf_ptr++ = *start;
	  break;      
      }
    } else {
      switch(*start) {
        case '\n':
          cool_yylval.error_msg = "Unterminated string constant";
	        curr_lineno++;
	        break;
        case '\0':
	        cool_yylval.error_msg = "String contains null character";
      	  return ERROR;
      	  break;
        case EOF:
      	  cool_yylval.error_msg = "String contains EOF character";
      	  return ERROR;
      	  break;
        default:
      	  *string_buf_ptr++ = *start;
      	  break;
      }
    }
    start++;
  }
  *string_buf_ptr = '\0';
  cool_yylval.symbol = stringtable.add_string(string_buf);
  return STR_CONST;
}

"=>"          { return DARROW; }
"<-"          { return ASSIGN; }
"<="          { return LE; }

"+"           { return '+'; }
"-"           { return '-'; }
"*"           { return '*'; }
"/"           { return '/'; }
"="           { return '='; }
"."           { return '.'; }
"~"           { return '~'; }
"@"           { return '@'; }
":"           { return ':'; }
";"           { return ';'; }
","           { return ','; }
"("           { return '('; }
")"           { return ')'; }
"{"           { return '{'; }
"}"           { return '}'; }
"<"           { return '<'; }


. {
    cool_yylval.error_msg = yytext;
    return ERROR;
}

%%
