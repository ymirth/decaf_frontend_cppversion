#include"Lexer.h"
//#include"scaner.h"
#include<iostream>
#include"parser.h"
using namespace std;
int main(){
	string finput = "input_string.txt";
	string foutput = "token.txt";
	string ferr = "error_lexer.txt";
	//finput = ferr;
	fstream fout(foutput, ios::out);
	cout << finput << endl;
	Scaner::scan(finput,fout);     //获取词法分析结果

	//获取语法分析结果
	Lexer::line = 1;
	Lexer* lex = new Lexer(finput);
	Parser* parser = new Parser(lex);
	parser->program();
	/*try{
		parser->program();
	}
	catch (string s) { cout << "error: "<<s; };*/
	
	//cout << "Finished"<<endl;// ("\n");
}