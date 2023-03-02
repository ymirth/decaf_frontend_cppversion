#include"Lexer.h"
extern Tag Stag;


const int Tag::AND = 256;
const int Tag::BASIC = 257;
const int Tag::BREAK = 258;
const int Tag::DO = 259;
const int Tag::ELSE = 260;
const int Tag::EQ = 261;
const int Tag::FALSE = 262;
const int Tag::GE = 263;
const int Tag::ID = 264;
const int Tag::IF = 265;
const int Tag::INDEX = 266;
const int Tag::LE = 267;
const int Tag::MINUS = 268;
const int Tag::NE = 269;
const int Tag::NUM = 270;
const int Tag::OR = 271;
const int Tag::REAL = 272;
const int Tag::TEMP = 273;
const int Tag::TRUE = 274;
const int Tag::WHILE = 275;
const int Tag::FOR = 276;


Word Word::AND("&&", Stag.AND);
Word Word::OR("||", Stag.OR);
Word Word::EQ("==", Stag.EQ);
Word Word::NE("!=", Stag.NE);
Word Word::LE("<=", Stag.LE);
Word Word::GE(">=", Stag.GE);
Word Word::MINUS("minus", Stag.MINUS);
Word Word::TRUE("true", Stag.TRUE);
Word Word::FALSE("false", Stag.FALSE);
Word Word::TEMP("t", Stag.TEMP);

int Lexer::line = 1;