#pragma once
#ifndef  INTER
#define INTER
#include"Lexer.h"
#include"symbol.h"
#include<iostream>
#include<fstream>

std::string fparse = "parse_result.txt";
fstream f_emit(fparse, ios::out);
class Node {
	int lexline;
	static int lables;
public:
	Node() :lexline(0) { this->lexline = Lexer::line; }

	void error(string s) {
		try {
			throw ("near line " + std::to_string(this->lexline) + ":" + s);
		}
		catch (string s) { cout << s << endl; }
	}
	
	static int newlabel() { return ++lables; }	//静态成员函数只能访静态成员，且没有this指针

	void emitlabel(int i) {
		f_emit << "L" << i << ":";// << endl;
	}

	void emit(string s) {
		f_emit << "\t" << s << endl;
	}
};

int Node::lables = 0;

class Expr :public Node {
public:
	Token* op;
	Type* type;

	Expr(Token* tok, Type* p) :
		op(tok), type(p) {}

	virtual Expr* gen() {
		return this;
	}

	virtual Expr* reduce() {
		return this;
	}

	virtual void jumping(int t, int f) {
		emitjumps(toString(), t, f);
	}
	void emitjumps(string test, int t, int f) {
		if (t != 0 && f != 0) {
			emit("if " + test + " goto L" + to_string(t));
			emit("goto L" + to_string(f));
		}
		else if (t != 0) emit("if " + test + " goto L" + to_string(t) );
		else if (f != 0) emit("if_false " + test + " goto L" + to_string(f));
		else{}  
	}

	virtual string toString() {
		return this->op->toString();
	}

};

class Stmt :public Node {
public:
	int after = 0;
public:
	static Stmt* Null() {
		static Stmt temp;
		return &temp;
	}
	static Stmt*& Enclosing() {
		static Stmt* temp = Null();
		return temp;
	}

	Stmt(){}

	virtual void gen(int b, int a) {}
	virtual void display(){
		cout << "Stmt base display" << endl;
	}


};
class Seq :public Stmt {
private:
	Stmt* stmt1;
	Stmt* stmt2;
public:
	Seq(Stmt* s1, Stmt* s2) :
		stmt1(s1), stmt2(s2) {}

	virtual void gen(int b, int a) {
		if (stmt1 == Stmt::Null()) stmt2->gen(b, a);
		else if (stmt2 == Stmt::Null()) stmt1->gen(b, a);
		else {
			int label = newlabel();
			stmt1->gen(b, label);
			emitlabel(label);
			stmt2->gen(label, a);
		}
	}

	virtual void display() {
		if (this->stmt1 == Stmt::Null()) {
			this->stmt2->display();
		}
		else if (this->stmt2 == Stmt::Null()) {
			this->stmt1->display();
		}
		else {
			this->stmt1->display();
			this->stmt2->display();
		}
	}
};
class Break :public Stmt {
	Stmt* stmt;

public :
	Break() {
		if (Stmt::Enclosing() == Stmt::Null()) {
			this->error("unenclosed break");
		}
		this->stmt = Stmt::Enclosing();
	}

	virtual void gen(int b,int a){
		emit("goto L" + to_string(stmt->after));
	}

	virtual void display() {
		this->emit(" break ");
	}
};

class Id :public Expr {
public:
	int offset;
	Id(Word* id, Type* p, int b) :
		Expr(id, p), offset(b) {}  // Word is sub-class of Token
};

class If :public Stmt {
	Expr* expr;
	Stmt* stmt;
public:
	If(Expr* x, Stmt* s) :
		expr(x), stmt(s) {
		if (this->expr->type != Type::Bool()) {
			this->expr->error("boolean request in if");
		}
	}
	virtual void gen(int  b, int a) {
		int label = newlabel();
		expr->jumping(0, a);
		emitlabel(label);
		stmt->gen(label, a);
	}
	virtual void display() {
		this->emit("stmt : if begin ");
		this->stmt->display();
		this->emit("stmt : if end ");
	}
};
class While :public Stmt {
private:
	Expr* expr = nullptr;
	Stmt* stmt = nullptr;
public:
	While() :expr(nullptr), stmt(nullptr) {}

	void init(Expr* x, Stmt* s) {
		this->expr = x;
		this->stmt = s;
		if (this->expr->type != Type::Bool()) {
			this->expr->error("boolean required in while");
		}
	}

	virtual void gen(int b, int a) {
		after = a;
		expr->jumping(0, a);
		int label = newlabel();
		emitlabel(label);
		stmt->gen(label, b);
		emit("goto L" + to_string(b));
	}

	virtual void display() {
		this->emit("stmt : while begin ");
		this->stmt->display();
		this->emit("stmt : while end ");
	}
};
class For:public Stmt {  // for(stmt1; condtion_expr; stmt2)
private:
	Expr* condition_expr = nullptr;
	Stmt* stmt = nullptr;

	Stmt* stmt1 = nullptr; 
	Stmt* stmt2 = nullptr;


public:
	For() :condition_expr(nullptr), stmt(nullptr) {}

	void init(Expr* x, Stmt* s,Stmt*s1,Stmt*s2) {
		this->condition_expr = x;
		this->stmt = s;
		this->stmt1 = s1;
		this->stmt2 = s2;
		if (this->condition_expr->type != Type::Bool()) {
			this->condition_expr->error("boolean required in for");
		}
	}

	virtual void gen(int b, int a) {
		
		after = a;
		stmt1->gen(b,a);                  // TAC for stmt1;
		int label_0 = newlabel();         
		emitlabel(label_0);               // L a+1
		condition_expr->jumping(0, a);    // if_false conditional_expr  goto L'a'
		int label = newlabel();	          // the label for the first instruction in the FOR loop
		emitlabel(label);				  // L a+2
		Stmt* loop = new Seq(stmt, stmt2);
		loop->gen(label, label_0);		  // TAC for the instructions in the FOR loop
		emit("goto L" + to_string(label_0));    // return to the loop head 



	}

	virtual void display() {
		this->emit("stmt : for begin ");
		this->stmt->display();
		this->emit("stmt : for end  ");
	}
};
class Set :public Stmt {
public:
	Id* id;
	Expr* expr;
	Set(Id* i, Expr* x) :id(i), expr(x) {
		if (this->check(this->id->type, this->expr->type) == nullptr) {
			this->error("type error");
		}
	}
	Type* check(Type* p1, Type* p2) {
		if (Type::numeric(p1) && Type::numeric(p2)) {
			return p2;
		}
		else {
			return p1 == Type::Bool() && p2 == Type::Bool() ? p2 : nullptr;
		}
	}
	virtual void gen(int b, int a) {
		emit(id->toString() + " = " + expr->gen()->toString());
	}
	virtual void display() {
		this->emit(" assignment ");
	}
};






class Constant :public Expr {
public:
	Constant(Token* tok, Type* p) :
		Expr(tok, p) {}
	explicit Constant(int i) :
		Expr(new Num(i), Type::Int()) {}
	static Constant* True() {
		static Constant temp(&Word::TRUE, Type::Bool());
		return &temp;
	}
	static Constant* False() {
		static Constant temp(&Word::FALSE, Type::Bool());
		return &temp;
	}
	virtual void jumping(int t, int f) {
		if (this == True() && t != 0) emit("goto L" + to_string(t));
		else if (this == False() && f != 0) emit("goto L" + to_string(f));
	}

};
class Do :public Stmt {
	Expr* expr = nullptr;  // Rel 类型 while里的 bool判断
	Stmt* stmt = nullptr;  // do后面要做的东西
public:
	Do() :expr(nullptr), stmt(nullptr) {}

	void init(Stmt* s, Expr* x) {	
		this->stmt = s;
		this->expr = x;
		if (this->expr->type != Type::Bool()) {
			this->expr->error("boolean required in do");
		}
	}
	virtual void gen(int b, int a) {
		after = a;
		int label = newlabel();
		stmt->gen(b, label);
		emitlabel(label);
		expr->jumping(b, 0);
	}

	virtual void display() {
		this->emit("stmt : do begin ");
		this->stmt->display();
		this->emit("stmt : do end ");
	}


};
class Else :public Stmt {
	Expr* expr;
	Stmt* stmt1;
	Stmt* stmt2;
public:
	Else(Expr* x, Stmt* s1, Stmt* s2) :
		expr(x), stmt1(s1), stmt2(s2) {
		if (this->expr->type != Type::Bool()) {
			this->expr->error("boolean request in if");
		}
	}
	virtual void gen(int  b, int a) {
		int label1 = newlabel();
		int label2 = newlabel();
		expr->jumping(0, label2);

		emitlabel(label1);
		stmt1->gen(label1, a);
		emit("goto L" + to_string(a));
		
		emitlabel(label2);
		stmt2->gen(label2, a);

	}
	virtual void display() {
		this->emit("stmt : else begin ");
		this->emit("if true ");
		this->stmt1->display();
		this->emit("else ");
		this->stmt2->display();
		this->emit("stmt : else end ");
	}
};

class Temp :public Expr {
protected:
	static int count;  //0
	int number;        //0
public:
	Temp(Type* p):Expr(&Word::TEMP,p) {
		this->number = ++count;
	}
	virtual string toString() {
		return "t" + to_string(this->number);
	}
};

int Temp::count = 0;

class Env {          // symbol table
private:
	unordered_map<Token*, Id*> table;
protected:
	Env* prev = nullptr;
public:
	Env(Env* n) :table(), prev(n) {}
	void put(Token* w, Id* i) {
		this->table[w] = i;
	}
	Id* get(Token* w) {
		for (Env* e = this; e != nullptr; e = e->prev) {
			if (e->table.find(w) != e->table.end()) {
				return e->table[w];
			}
		}

		return nullptr;
	}
};


class Op :public Expr {
public:
	Op(Token* tok, Type* p) :Expr(tok, p) {}

	virtual Expr* reduce() {
		Expr* x = this->gen();
		Temp* t = new Temp(this->type);
		this->emit(t->toString() + "=" + x->toString());
		return t;
	}
};
class Arith :public Op {
public:
	Expr* expr1;
	Expr* expr2;
	Arith(Token* tok, Expr* x1, Expr* x2):
		Op(tok,(Type*) nullptr) ,
		expr1(x1),expr2(x2)
	{
		this->type = Type::max(static_cast<const Type*>(this->expr1->type), static_cast<const Type*>(this->expr2->type));
		if (this->type == nullptr) {
			this->error("type error");
		}
	}
	virtual Expr* gen() {
		return new Arith(op, expr1->reduce(), expr2->reduce());
	}
	virtual string toString() {
		return expr1->toString() + " " + op->toString() + " " + expr2->toString();
	}
};
class Access :public Op {
public:
	Id* array;
	Expr* index;

	Access(Id* a, Expr* i, Type* p):
		Op(new Word("[]",266), p),
		array(a),index(i){}

	virtual Expr* gen() {
		return new Access(this->array, this->index->reduce(), this->type);
	}

	virtual void jumping(int t, int f){
		emitjumps(reduce()->toString(), t, f);
	}

	string toString() {
		return  this->array->toString() + " [ " + this->index->toString() + " ] ";
	}
};
class Unary :public Op {
public:
	Expr* expr = nullptr;
	Unary(Token* tok, Expr* x) :
		Op(tok, (Type *)nullptr),
		expr(x) {
		this->type = Type::max(Type::Int(), this->expr->type);
		if (this->type == nullptr) {
			this->error("type error");
		}
	}
	virtual Expr* gen() {
		return new Unary(this->op, this->expr->reduce());
	}
	virtual string toString() {
		return op->toString() + " " + expr->toString();
	}
};

class SetElem :public Stmt {
public:
	Id* array;
	Expr* index;
	Expr* expr;

	SetElem(Access* x, Expr* y) {
		array = x->array; index = x->index; expr = y;
		if (check(x->type, expr->type) == nullptr) error("type error");
	}

	Type* check(Type* p1, Type* p2) {
		if (!(instanceof<Array>(p1)) && !(instanceof<Array>(p2))) {
			if (p1 == p2) {
				return p2;
			}
			else {
				return Type::numeric(p1) && Type::numeric(p2) ? p2 : nullptr;
			}
		}
		else {
			return nullptr;
		}
	}
	template<typename Base, typename T>
	inline bool instanceof(const T*) {
		return is_base_of<Base, T>::value;
	}

	virtual void gen(int b, int a) {
		string s1 = index->reduce()->toString();
		string s2 = expr->reduce()->toString();
		emit(array->toString() + " [ " + s1 + " ] = " + s2);
	}
	virtual void display() {
		this->emit(" assignment ");
	}
};

class Logical :public Expr {
public:
	Expr* expr1;
	Expr* expr2;

	Logical(Token* tok, Expr* x1, Expr* x2) :
		Expr(tok, (Type*)nullptr),
		expr1(x1), expr2(x2) {
		this->type = this->check(this->expr1->type, this->expr2->type);
	}

	virtual Type* check(Type* p1, Type* p2) {
		return p1 == Type::Bool() && p2 == Type::Bool() ? Type::Bool() : nullptr;
	}

	virtual Expr* gen() {
		int f = this->newlabel();
		int a = this->newlabel();
		Temp* temp = new Temp(this->type);
		this->jumping(0, f);
		this->emit(temp->toString() + " = true");
		this->emit("goto L" + to_string(a));
		this->emitlabel(f);
		this->emit(temp->toString() + " = false");
		this->emitlabel(a);
		return temp;
	}
};
class And:public Logical {
public:
	And(Token* tok, Expr* x1,Expr* x2):Logical(tok,x1,x2){}
	virtual void jumping(int t, int f) {
		int label = f != 0 ? f : newlabel();
		expr1->jumping(0, label);
		expr2->jumping(t, f);
		if (f == 0) emitlabel(label);
	}
};
class Or :public Logical {
public:
	Or(Token* tok, Expr* x1, Expr* x2) :Logical(tok, x1, x2) {}
	virtual void jumping(int t, int f) {
		int label = t != 0 ? t : newlabel();
		expr1->jumping(label,0);
		expr2->jumping(t, f);
		if (t == 0) emitlabel(label);
	}
};
class Not :public Logical {
public:
	Not(Token* tok, Expr* x2) :Logical(tok, x2, x2) {}
	virtual void jumping(int t, int f) {
		expr2->jumping(f, t);
	}
	virtual string toString() {
		return this->op->toString() + " " + this->expr2->toString();
	}
};
class Rel :public Logical {
public:
	Rel(Token* tok, Expr* x1, Expr* x2) :Logical(tok, x1, x2) {
		this->type = check(x1->type, x2->type);
	}
	virtual void jumping(int t, int f) {
		Expr* a = expr1->reduce();
		Expr* b = expr2->reduce();

		string test = a->toString() + " " + op->toString() + " " + b->toString();
		emitjumps(test, t, f);
	}

	virtual Type* check(Type* p1, Type* p2) {
		if (!(instanceof<Array>(p1)) && !(instanceof<Array>(p2))) {
			return p1 == p2 ? Type::Bool() : nullptr;
		}
		else {
			return nullptr;
		}
	}
	template<typename Base, typename T>
	inline bool instanceof(const T*) {
		return is_base_of<Base, T>::value;
	}
};



#endif // ! INTER

