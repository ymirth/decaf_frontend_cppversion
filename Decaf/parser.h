#pragma once
#ifndef PARSER
#define PARSER
#include"inter.h"
#include"Lexer.h"
class Parser {
private:
	Lexer* lex;			  // lexer to get next token
	Token* look;		  // lookahead token
	Env* top = nullptr;   // current or top symbol table
	int used = 0;		  // storage used for declarations
public:
	explicit Parser(Lexer* l)throw(IOException) :lex(l) {
		move();
	}
	void move()throw(IOException) { // next token
		look = lex->scan();
		//cout << look->toString() << endl;
	}// next token

	void error(string s)throw(string) {
		try {
			throw ("near line" + to_string(lex->line) + ": " + s);
		}
		catch (string s) { cout << s; }
	}

	void match(int t)throw(IOException) {
		if (look->tag == t) move();
		else error("synatx error");
	}

	void program()throw(IOException) {
		// build the syntax tree
		Stmt* s = block();
		// display the syntax tree
		// only display the stmts, without expr
		s->display();

		int begin = s->newlabel();
		int after = s->newlabel();

		s->emitlabel(begin);
		s->gen(begin, after);
		s->emitlabel(after);
	}

	Stmt* block() throw(IOException) {  // block -> { decls stmts }
		match('{');
		Env* savedEnv = top;		// Env
		top = new Env(top);			//Env
		decls(); Stmt* s = stmts();
		match('}');
		top = savedEnv;				// Env 链表头指针
		return s;
	}
	void decls() throw(IOException) { //decls ->decls decl | epslon
		while (look->tag == Tag::BASIC) {   // decl -> type ID ;
			Type* p = type(); Token* tok = look; match(Tag::ID); match(';');
			Id* id = new Id((Word*)tok, p, used);	//ID node
			top->put(tok, id);						// record in symbol table
			used = used + p->width;
		}
	}
	Type* type() throw(IOException){
			Type* p = (Type*)look;            // expect look->tag == Tag::BASIC 
			match(Tag::BASIC);
			if (look->tag != '[') return p;   // T -> basic : int float char bool
			else return dims(p);              // return array type
	}
	
	Type* dims(Type* p) throw(IOException) { //array -> array[num]
			match('[');  Token* tok = look;  match(Tag::NUM);  match(']');
			if (look->tag == '[') 
				p = dims(p);
			return new Array(((Num*)tok)->value, p);  //Array(size,type)
		
	}
	Stmt* stmts() throw(IOException) { //stmts -> stmts stmt | epslon
		if (look->tag == '}') return Stmt::Null();
		else {
			Stmt* stmt1 = stmt();
			Stmt* stmt2 = stmts();
			return new Seq(stmt1, stmt2);
			//return new Seq(stmt(), stmts());  错误，编译器可能先执行stmts()从而导致无限递归
		}
	}

	Stmt* stmt() throw( IOException) {
		Expr* x;  Stmt *s, *s1, *s2;
		Stmt* savedStmt;         // save enclosing loop for breaks

		switch (look->tag) {

		case ';': { // stmt-> ;
			move();
			return Stmt::Null();
			break; 
		}
		case 265: {//Tag::IF: // stmt -> if (bool) stmt | if (bool) stmt else stmt
			match(Tag::IF); 
			match('('); 
			x = _bool_(); 
			match(')');
			s1 = stmt();
			if (look->tag != Tag::ELSE) return new If(x, s1);
			match(Tag::ELSE);
			s2 = stmt();
			return new Else(x, s1, s2);
			break;
		}
		case 275://Tag::WHILE: //stmt->while(bool) stmt
		{
			While* whilenode = new While();
			savedStmt = Stmt::Enclosing(); Stmt::Enclosing() = whilenode;
			match(Tag::WHILE); match('('); x = _bool_(); match(')');
			s1 = stmt();
			whilenode->init(x, s1);
			Stmt::Enclosing() = savedStmt;  // reset Stmt.Enclosing
			return whilenode;
			break;
		}
		case 259://Tag::DO:
		{
			Do* donode = new Do();
			savedStmt = Stmt::Enclosing(); Stmt::Enclosing() = donode;
			match(Tag::DO);
			s1 = stmt();
			match(Tag::WHILE); 
			match('('); 
			x = _bool_();
			match(')'); 
			match(';');
			donode->init(s1, x);
			Stmt::Enclosing() = savedStmt;  // reset Stmt.Enclosing
			return donode;
			break;
		}
		case 258://Tag::BREAK:
			match(Tag::BREAK); match(';');
			return new Break();
			break;
		case 276: {//Tag:FOR
			For* fornode = new For();
			savedStmt = Stmt::Enclosing(); Stmt::Enclosing() = fornode;
			match(Tag::FOR); 
			match('(');
			s1 = stmt();   // assign
			
			x = _bool_();  // condition 
			match(';');
			s2 = stmt();   // assign
			match(')');
			s = stmt();
			fornode->init(x, s,s1,s2);
			Stmt::Enclosing() = savedStmt;  // reset Stmt.Enclosing
			return fornode;
			break;
		}
		case '{':
			return block();
			break;

		default:
			return assign();
		}
	}
	
	Stmt* assign() throw(IOException) {
		Stmt* stmt;  Token* t = look;
		match(Tag::ID);
		Id* id = top->get(t);
		if (id == nullptr) error(t->toString() + " undeclared");

		if (look->tag == '=') {       // S -> id = E ;
			move();  stmt = new Set(id, _bool_());
		}
		else {                        // S -> L = E ;
			Access* x = offset(id);
			match('=');  stmt = new SetElem(x, _bool_());
		}
		match(';');
		return stmt;
	}

	
	Expr* _bool_() throw(IOException) {
		Expr* x = join();
		while (look->tag == Tag::OR) {
			Token* tok = look;  move();  x = new Or(tok, x, join());
		}
		return x;
	}

	Expr* join() throw(IOException) {
		Expr* x = equality();
		while (look->tag == Tag::AND) {
			Token* tok = look;  move();  x = new And(tok, x, equality());
		}
		return x;
	}

	Expr* equality() throw(IOException) {
		Expr* x = rel();
		while (look->tag == Tag::EQ || look->tag == Tag::NE) {
			Token* tok = look;  move();  
			x = new Rel(tok, x, rel());
		}
		return x;
	}

	Expr* rel() throw(IOException) {
		Expr* x = expr();
		switch (look->tag) {
		case '<': {
			Token* tok = look;  move();  
			return new Rel(tok, x, expr());
			break;
		}
		case 267/*Tag::LE*/: {
			Token* tok = look; 
			move();  
			move();
			return new Rel(tok, x, expr());
			break;
		}
		case 263/*Tag::GE*/: {
			Token* tok = look;  
			move(); 
			move();
			Expr* x2 = expr();
			return new Rel(tok, x, x2);
			break;
		}
		case '>': {
			Token* tok = look;  move();  return new Rel(tok, x, expr());
			break;
		}
		default:
			return x;
		}
	}

	Expr* expr() throw(IOException) {
		Expr* x = term();
		while (look->tag == '+' || look->tag == '-') {
			Token* tok = look;  move();  x = new Arith(tok, x, term());
		}
		return x;
	}

	Expr* term() throw(IOException) {
		Expr* x = unary();
		while (look->tag == '*' || look->tag == '/') {
			Token* tok = look;  move();   x = new Arith(tok, x, unary());
		}
		return x;
	}

	Expr* unary() throw(IOException) {
		if (look->tag == '-') {
			move();  return new Unary(&Word::MINUS, unary());
		}
		else if (look->tag == '!') {
			Token* tok = look;  move();  return new Not(tok, unary());
		}
		else return factor();
	}

	Expr* factor() throw(IOException) {
		Expr* x = nullptr;
		switch (look->tag) {
		case '(':
			move(); x = _bool_(); match(')');
			return x;
		case 270://Tag::NUM:
			x = new Constant(look, Type::Int());    move(); return x;
		case 272://Tag::REAL:
			x = new Constant(look, Type::Float());  
			move(); 
			return x;
		case 274://Tag::TRUE:
			x = Constant::True();                   move(); return x;
		case 262://Tag::FALSE:
			x = Constant::False();                  move(); return x;
		default:
			error("syntax error");
			return x;
		case 264://Tag::ID:
			string s = look->toString();
			Id* id = top->get(look);
			if (id == nullptr) error(look->toString() + " undeclared");
			move();
			if (look->tag != '[') return id;
			else return offset(id);
		}
	}

	Access* offset(Id* a) throw(IOException) {   // I -> [E] | [E] I
		Expr* i; Expr* w; Expr* t1, *t2; Expr *loc;  // inherit id

		Type* type = a->type;
		match('['); i = _bool_(); match(']');     // first index, I -> [ E ]
		type = ((Array*)type)->of;
		w = new Constant(type->width);
		t1 = new Arith(new Token('*'), i, w);
		loc = t1;
		while (look->tag == '[') {      // multi-dimensional I -> [ E ] I
			match('['); i = _bool_(); match(']');
			type = ((Array *)type)->of;
			w = new Constant(type->width);
			t1 = new Arith(new Token('*'), i, w);
			t2 = new Arith(new Token('+'), loc, t1);
			loc = t2;
		}

		return new Access(a, loc, type);
	}

};

#endif // !PARSER
