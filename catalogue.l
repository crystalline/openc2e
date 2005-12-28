%option noyywrap
%option nodefault
%option yylineno
%x CAT_STRING
    #include "Catalogue.h"
    #include <cstring>
    #include "catalogue.tab.hpp"
    static std::string cat_str;
%%
\#[^\n\r]*[\n\r]*   // comment
TAG                 return CTOK_TAG;
ARRAY               return CTOK_ARRAY;
OVERRIDE            return CTOK_OVERRIDE;
[0-9]+              { catalval.number = atoi(yytext); return CTOK_INT; }
\"                  cat_str = ""; BEGIN(CAT_STRING);
<CAT_STRING>\\.     cat_str += catalogue_descape(yytext[1]);
<CAT_STRING>\"      BEGIN(INITIAL); catalval.string = new std::string(cat_str); return CTOK_STR;
<CAT_STRING>.       cat_str += yytext[0];
[ \t\n\r]           // yum, whitespace