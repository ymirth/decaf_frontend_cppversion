#pragma once
#ifndef LEXER
#define LEXER

#include<iostream>
#include<unordered_map>
#include<fstream>
#include<string>
#include<iostream>
using namespace std;



class Tag {
public:
    const static int AND;
    const static int BASIC;
    const static int BREAK;
    const static int DO;
    const static int ELSE;
    const static int EQ;
    const static int FALSE;
    const static int GE;
    const static int ID;
    const static int IF;
    const static int INDEX;
    const static int LE;
    const static int MINUS;
    const static int NE;
    const static int NUM;
    const static int OR;
    const static int REAL;
    const static int TEMP;
    const static int TRUE;
    const static int WHILE;
    const static int FOR;
    Tag() {}
};


class Token {
public:
    int tag;
    string lexeme;
    Token():tag(0) {}
    explicit Token(const int t) {
        tag = t;
    }
    explicit Token(const string s) {
        lexeme = s;
        tag = s[0];
    }
    explicit Token(const char s) {
        lexeme.resize(1);
        lexeme[0] = s;
        tag = s;
    }
    virtual string toString() {
        //char c = char(tag);
        //cout << "tag is " << tag << " " << "char is" << c << endl;
        return  lexeme;
        //return to_string(tag);
    }
};

class Word :public Token {
public:
    string lexeme;

    static Word AND;
    static Word OR;
    static Word EQ;
    static Word NE;
    static Word LE;
    static Word GE;
    static Word MINUS;
    static Word TRUE;
    static Word FALSE;
    static Word TEMP;

public:
    Word() :Token() {}
    Word(string s, int tag) : Token(tag), lexeme(s) {}
    string toString() { return this->lexeme; }
};

class Num :public Token {
public:
    int value;
    Tag Stag;
    explicit Num(int v) :Token(Stag.NUM) {
        value = v;
    }
    string toString() { return std::to_string(value); }
};

class Real :public Token {
public:
    float value;
    Tag Stag;
    explicit Real(float v) :Token(Stag.REAL) {
        value = v;
    }
    string toString() { return std::to_string(value); }
};
class IOException {
public:
    IOException() :line(0){}
    explicit IOException(string s) :method(s),line(0) { what(); }
    IOException(string s,int l) :method(s),line(l) { what(); }
public:

public:
    void what() const {  //获取具体的错误信息
        cerr << "Method: " << method << "is paused at line" << line << endl;
    }
private:
    string method;
    int line;
};

class Type :public Word {
public:
    int width; // 0

    Type(string s, int tag, int w) :
        Word(s, tag), width(w) {}     // TAG IS Tag::BASIC

    static Type* Int() {     //Effective C++ : Rule 04
        static Type temp("int", 257, 4);
        return &temp;
    }
    static Type* Float() {   //Effective C++ : Rule 04
        static Type temp("float", 257, 8);
        return &temp;
    }
    static Type* Char() {   //Effective C++ : Rule 04
        static Type temp("char", 257, 1);
        return &temp;
    }
    static Type* Bool() {   //Effective C++ : Rule 04
        static Type temp("bool", 257, 1);
        return &temp;
    }

    static bool numeric(const Type* p) {
        return p == Char() || p == Int() || p == Float();
    }



    static Type* max(const Type* p1, const Type* p2) {
        if (numeric(p1) && numeric(p2)) {                          //为数值类型
            if (p1 != Float() && p2 != Float()) {					   //均不为浮点类型
                return p1 != Int() && p2 != Int() ? Char() : Int();    //若均不为整数，则返回 字符类型； 反之 返回整数类型
            }
            else {
                return Float();										   //返回浮点类型 
            }
        }
        else
        {
            return nullptr;										  //不为数值类型，返回空指针
        }
    }



};

class Array :public Type {
public:
    Type* of = nullptr;
    int size = 1;
    Array(const int& sz, Type* p) :
        Type("[]", 266, sz* p->width),
        size(sz),
        of(p) {}
    string toString() {
        return "[" + to_string(this->size) + "]" + this->of->toString();
    }
};

class Lexer {
private:
    string inputFile;
    fstream inFile;
public:
    static int line;
    Tag Stag;
    char peek = ' ';
    unordered_map<string, Word*> words;

    void reserve(Word* w) { words[w->lexeme] = w; }

    Lexer() {
        reserve(new Word("if", Stag.IF));
        reserve(new Word("else", Stag.ELSE));
        reserve(new Word("while", Stag.WHILE));
        reserve(new Word("do", Stag.DO));
        reserve(new Word("break", Stag.BREAK));
        reserve(&(Word::TRUE));
        reserve(&(Word::FALSE));
        /*cout << "ni" << endl;*/
    }
    Lexer(string input) :inputFile(input) {
        reserve(new Word("if", Stag.IF));
        reserve(new Word("else", Stag.ELSE));
        reserve(new Word("while", Stag.WHILE));
        reserve(new Word("for", Stag.FOR));
        reserve(new Word("do", Stag.DO));
        reserve(new Word("break", Stag.BREAK));
        reserve(Type::Int());
        reserve(Type::Float());
        reserve(Type::Char());
        reserve(Type::Bool());
        reserve(&(Word::TRUE));
        reserve(&(Word::FALSE));
        //cout << "ni2" << endl;
        inFile.open(inputFile);
        if (!inFile.is_open()) {
            cerr << "Could not find the file\n";
            cerr << "Program terminating\n";
            abort();
        }
        inFile >> noskipws;
    }


    bool state() {
        if (inFile.is_open()) return true;
        else return false;
    }
    bool readch()throw (IOException) {
        
        if (!inFile.is_open()) {
            if(peek =='#')
                throw IOException("readch() in scan ", line);
            peek = '#';
            return false;

        }
        if (!inFile.eof()) {
            //char temp;
            
            inFile >> peek;
            //cout << peek;
            if (!inFile.eof()) return true;
            else
            {
                inFile.close();
                return false;
            }
        }
        else {
            inFile.close();
            return false;
        }
      
        //else system("Pause");
     
    }
    bool readch(char c)throw(IOException) {
        readch();
        if (peek != c) {
            return false;
        }
        else return true;
    }
    Token* scan() throw(IOException) {
        for (;; readch()) {
            if (peek == ' ' || peek == '\t')
                continue;
            else if (peek == '\n')
                line += 1;
            else if (peek == '#') break;
            else break;
        }
        switch (peek)
        {
        case '&':
            if (readch('&'))
                return &Word::AND;
            else
                return new Token("&");
        case '|':
            if (readch('|'))
                return &Word::OR;
            else
                return new Token("|");
        case '=':
            if (readch('='))
                return &Word::EQ;
            else
                return new Token("=");
        case '!':
            if (readch('='))
                return &Word::NE;
            else
                return new Token("!");
        case '<':
            if (readch('='))
                return &Word::LE;
            else
                return new Token("<");
        case '>':
            if (readch('='))
                return &Word::GE;
            else
                return new Token(">");
        default:
            break;
        }
        if (isdigit(peek)) {
            int v = 0;
            bool state;
            do {
                v = 10 * v + int(peek - '0');
                state = readch();
                
            } while (isdigit(peek) && state == true);
            if (peek != '.')
                return new Num(v);
            float x = v;
            float d = 10;
            for (;;) {
                readch();
                if (!isdigit(peek))
                    break;
                x = x + int(peek - '0') / d;//Character.digit
                d = d * 10;
            }
            return new Real(x);
        }
        if (isalpha(peek)) {
            string s;
            bool state;
            do {
                s += peek;
                state = readch();
            } while ((isalpha(peek) || isdigit(peek))&& state==true);


            if (words.find(s) != words.end())
                return words[s];
            else {
                Word* w = new Word(s, Stag.ID);
                words[s] = w;
                return w;
            }

        }
        Token* tok  = new Token(peek);
        peek = ' ';
        return tok;

    }

    void out() {
        cout << words.size();
    }

    char getPeek() {
        return peek;
    }

    void setPeek(char peek) {
        this->peek = peek;
    }

    ~Lexer() {
        if (inFile.is_open())
            inFile.close();
    }
};

class Scaner {
public:
    static void scan(string input,fstream& out) throw(IOException) {
        Lexer lexer(input);
        char c;
        try {
            while (lexer.state() == true) {
                do {
                    Token* token = lexer.scan();
                    //cout << lexer.getPeek() << endl;
                    switch (token->tag) {
                    case 270:
                    case 272:
                        out << "line "<<lexer.line<<" :" << "(NUM , " + token->toString() + ")" << endl;
                        break;
                    case 264:
                        out << "line " << lexer.line << " :" << "(ID , " + token->toString() + ")" << endl;
                        break;
                    case 256:
                    case 257:
                    case 258:
                    case 259:
                    case 260:
                    case 265:
                    case 274:
                    case 275:
                        out << "line " << lexer.line << " :" << "(KEY , " + token->toString() + ")" << endl;
                        break;
                    case 13:
                        break;
                    default:
                        out << "line " << lexer.line << " :" << "(SYM , " + token->toString() + ")" << endl;
                        break;
                    }

                } while (lexer.getPeek() != '\n' && lexer.state() == true);
            }      
        }
        catch (IOException x) {
            cout << "Finished" << endl;
        }

    }

};


#endif // !LEXER