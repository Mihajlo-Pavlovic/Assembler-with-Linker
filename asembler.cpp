#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <sstream>
#include <vector>
#include <regex>
#include <math.h>
#include "tables.hpp"

#define R_386_16 1
#define R_386_PC16 2

#define LABEL_SYMBOL 1
#define SYMBOL_SYMBOL 2
#define SECTION_SYMBOL 3
#define EXTERN_SYMBOL 4


class InstructionDescriptor
{
public:
	std::string name;
	int minSize;
	int maxSize;
	int numOperators;

	InstructionDescriptor(std::string name, int maxSize, int minSize, int numOperators) : name{name},
																						  maxSize{maxSize},
																						  minSize{minSize},
																						  numOperators{numOperators}
	{
	}
};

//srediti $,%,*[] u regeksima
std::regex labelRegex("^([a-zA-Z_][a-zA-Z0-9_]*_{0,}):$");
std::regex directiveRegex("^\\.(align|byte|equ|skip|word|end|global|extern|section)$");
std::regex symbolRegex("^[a-zA-Z_][a-zA-Z0-9_]*$");
//registri
std::regex registerRegex = std::regex("^r[0-9]$"), // register direct addressing
	regOffLitRegex("^\\[r[0-7](\\-|\\+)([1-9][0-9]*|0b[0-1]+|0[0-7]+|0[x][0-9a-fA-F]+)\\]$"),
		   regOffSymRegex("^\\[r[0-7](\\-|\\+)([a-zA-Z_][a-zA-Z0-9_]*)\\]$");
// //literali
std::regex decimalRegex("(-){0,1}0|[1-9][0-9]*"),
	binaryRegex("0[b][0-1]+"),
	octRegex("0[0-7]+"),
	hexRegex("0[x][0-9a-fA-F]+"),
	literalRegex("^((-){0,1}0|[1-9][0-9]*|0[b][0-1]+|0[0-7]+|0[x][0-9a-fA-F]+)$");
//instrukcije
std::regex instructionRegex("^halt|int|iret|call|ret|jmp|jeq|jne|jgt|push|pop|xchg|add|sub|mul|div|cmp|not|and|or|xor|test|shl|shr|ldr|str$");
//instrukcije jump;
std::regex jumpInstruction("jmp|jeq|jne|jgt|call");
//no opr instruction
std::regex noOperandInstruction("halt|iret|ret");
//single reg instruction
std::regex singleRegInstruction("int|not");
//two reg instruction
std::regex twoRegInstruction("xchg|add|sub|mul|div|cmp|and|or|xor|test|shl|shr");
//load|store
std::regex ldStr("ldr|str");

//operator svaki (fale oni prefiksi)
std::regex operatorRegex("^[$%*\\[]{0,1}(([a-zA-Z_][a-zA-Z0-9_]*)|(\\[r[0-7](\\-|\\+)([1-9][0-9]*|0b[0-1]+|0[0-7]+|0[x][0-9a-fA-F]+|[a-zA-Z_][a-zA-Z0-9_]*)\\])|((-){0,1}0)|([1-9][0-9]*|0[b][0-1]+)|(0[0-7]+)|(0[x][0-9a-fA-F]+))[\\]]{0,1}$");

//nejump operandi

enum Token_type
{
	ILLEGAL = -1,
	LABEL,
	DIRECTIVE,
	INSTRUCTION,
	OPERATOR
};

enum Operator
{
	LIT_IMM_OPR = 0,
	SIM_IMM_OPR,
	LIT_DIR_OPR,
	SIM_DIR_OPR,
	SIM_REL_OPR,
	REG_IMM_OPR,
	REG_DIR_OPR,
	REG_DIR_OFFLIT_OPR,
	REG_DIR_OFFSIM_OPR,
	JUMP_LIT_DIR_OPR,
	JUMP_SIM_DIR_OPR,
	JUMP_REG_IMM_OPR,
	JUMP_REG_DIR_OPR,
	JUMP_REG_DIR_OFFLIT_OPR,
	JUMP_REG_DIR_OFFSIM_OPR
};

enum Directive
{
	DIR_GLB = 0,
	DIR_EXT,
	DIR_SEC,
	DIR_WOR,
	DIR_SKI,
	DIR_EQU,
	DIR_END
};

enum Instruction
{
	HALT = 0,
	INT,
	IRET,
	CALL,
	RET,
	JMP,
	JEQ,
	JNE,
	JGT,
	PUSH,
	POP,
	XCHG,
	ADD,
	SUB,
	MUL,
	DIV,
	CMP,
	NOT,
	AND,
	OR,
	XOR,
	TEST,
	SHL,
	SHR,
	LDR,
	STR
};
InstructionDescriptor instructionDescriptors[] = {
	InstructionDescriptor("halt", 1, 1, 0),
	InstructionDescriptor("int", 2, 2, 0),
	InstructionDescriptor("iret", 1, 1, 0),
	InstructionDescriptor("call", 5, 3, 1),
	InstructionDescriptor("ret", 1, 1, 0),
	InstructionDescriptor("jmp", 5, 3, 1),
	InstructionDescriptor("jeq", 5, 3, 1),
	InstructionDescriptor("jne", 5, 3, 1),
	InstructionDescriptor("jgt", 5, 3, 1),
	//ovo mora posebno da se obradi--------------
	InstructionDescriptor("push", 3, 3, 1), //  |
	InstructionDescriptor("pop", 3, 3, 1),	//	|
	//-------------------------------------------
	InstructionDescriptor("xchg", 2, 2, 2),
	InstructionDescriptor("add", 2, 2, 2),
	InstructionDescriptor("sub", 2, 2, 2),
	InstructionDescriptor("mul", 2, 2, 2),
	InstructionDescriptor("div", 2, 2, 2),
	InstructionDescriptor("cmp", 2, 2, 2),
	InstructionDescriptor("not", 1, 2, 1),
	InstructionDescriptor("and", 2, 2, 2),
	InstructionDescriptor("or", 2, 2, 2),
	InstructionDescriptor("xor", 2, 2, 2),
	InstructionDescriptor("test", 2, 2, 2),
	InstructionDescriptor("shl", 2, 2, 2),
	InstructionDescriptor("shr", 2, 2, 2),
	InstructionDescriptor("ldr", 5, 3, 2),
	InstructionDescriptor("str", 5, 3, 2),
};
class Token
{
public:
	std::string value;
	Token_type tokenType;

public:
	Token(std::string value, Token_type tokenType = Token_type::ILLEGAL) : value{value},
																		   tokenType{tokenType}
	{
	}

	std::string getValue()
	{
		return value;
	}

	// void setValue(std::string value){
	// 	this->value = value;
	// }

	Token_type getTokenType()
	{
		return tokenType;
	}

	// void setTokenType(Token_type tokenType){
	// 	this->tokenType = tokenType;
	// }
};

std::vector<Token> tokenizeLine(std::string line, int lineNumber)
{
	std::vector<Token> tokenizedLine;
	/*
	while(int pos = line.find('\t') != std::string::npos){
		line.erase(pos-1, pos);
		line.insert(pos-1, " ");
	}
	*/
	std::stringstream ss;
	ss << line;
	std::string value;
	Token_type tt;
	while (std::getline(ss, value, ' '))
	{
		// std::cout << lineNumber << '.';
		//videti neki pametniji nacin za ovo
		if (value.at(value.size() - 1) == ',')
		{
			value.erase(value.size() - 1);
		}
		while (int pos = value.find('\t') != std::string::npos)
		{
			value.erase(pos - 1, pos);
			// value.insert(pos-1, " ");
		}
		if (std::regex_match(value, labelRegex))
		{
			tokenizedLine.push_back(Token(value, Token_type::LABEL));
			// std::cout << value << " label ";
		}
		else if (std::regex_match(value, directiveRegex))
		{
			tokenizedLine.push_back(Token(value, Token_type::DIRECTIVE));
			// std::cout << value << " directive ";
		}
		else if (std::regex_match(value, instructionRegex))
		{
			tokenizedLine.push_back(Token(value, Token_type::INSTRUCTION));
			// std::cout << value << " instruction ";
		}
		else if (std::regex_match(value, operatorRegex))
		{
			tokenizedLine.push_back(Token(value, Token_type::OPERATOR));
			// std::cout << value << " operator ";
		}
		else
		{
			tokenizedLine.push_back(Token(value));
			std::cout << lineNumber << '.';
			std::cout << value << " ILLEGAL ";
			std::cout << std::endl;
		}
	}
	return tokenizedLine;
}

std::string toLittleEndianHex(int value){
  // convert to long little endian hex
//   int num = __builtin_bswap16(value);
  // convert to string
	std::ostringstream oss;
	oss << std::hex << value;
	std::string mystring = oss.str();
	for(int i = mystring.size(); i < 4; i++)
		mystring.insert(0,"0");
	std::string ret;

	ret = mystring.substr(2,2);
	ret.append(mystring.substr(0,2));


  return ret;
}
std::string createRowDescription(std::string name, int value){
	std::stringstream  descstream;
	descstream << "symbolic name to a numeric constant " << name << "=" << std::to_string(value) << "\n";
	std::string description = descstream.str();
	return description;
}
int main(int argc, char *argv[])
{
	// std::cout << argc << " " << argv[1] << std::endl;
	std::ifstream input_file;
	std::ofstream txt_output_file;
	std::string input = "input.s";
	std::string output = argv[1];
	input_file.open(input, std::ifstream::in);
	txt_output_file.open(output, std::ofstream::out | std::ofstream::trunc);

	if (!input_file.is_open())
		std::cout << "Greska pri otvaranj inputa";
	else if (!txt_output_file.is_open())
		std::cout << "Greska pri otvaranj outputa " << output << std::endl;


	std::vector<std::vector<Token>> tokenizedFile;
	std::vector<SectionTable> sectionTables;
	SymbolTable symbolTable;
	RelocationTable relocationTable;
	//Absolute table postoji uvek za .equ deklarisane sibmole
	SectionTable absoluteSection("Absolute", 0);
	std::string line;
	int lineNumber = 0;
	//pomocne
	int value;
	std::string name;
	int alocatedSpace;
	//OVO SREDITI
	std::string curSection = "NULL";
	int locationCounter = 0;
	bool endFound = false;
	//uzim red i odbacuje \n
	while (std::getline(input_file, line) && !endFound)
	{
		lineNumber++;
		//da li treba da se sredi da se sp,pc,psw prebaci u r8,9...
		//to lowercase
		std::transform(line.begin(), line.end(), line.begin(), [](unsigned char c)
					   { return std::tolower(c); });
		//delete comments
		int deleteFrom = line.find_first_of('#');
		if (deleteFrom != std::string::npos)
			line = line.erase(deleteFrom); 
		//spoji sve oko plusa
		int mergeOffset = 0;
		while (mergeOffset != std::string::npos)
		{
			mergeOffset = line.find('+', mergeOffset);
			if (mergeOffset != std::string::npos)
			{
				if (mergeOffset != 0 && line.at(mergeOffset - 1) == ' ')
				{
					line.erase(mergeOffset - 1, 1);
					mergeOffset--; // + se pomerio na mesto iza jer smo obrisali karakter iza njega
				}
				if (line.at(mergeOffset + 1) == ' ')
					line.erase(mergeOffset + 1, 1);
				mergeOffset++;
			}
		}
		//tokenize
		std::vector<Token> tokenizedLine = tokenizeLine(line, lineNumber);
		tokenizedFile.push_back(tokenizedLine);
		// obrada tokena
		bool lineProcessed = false;
		int instructionID = -1;
		int numOperands;
		bool labeldLine = false;

		for (int i = 0; i < tokenizedLine.size() && !lineProcessed; i++)
		{
			Token currentToken = tokenizedLine[i];
			switch (currentToken.getTokenType())
			{
			case Token_type::LABEL:
				//proveri jel ovo jedini label u liniji i da li je na pocetku
				//proveri flag jel ovo prvi put da se definise ovaj label, ovo deligirao na insert u tabelu
				if (labeldLine == true || i != 0)
				{
					std::cout << lineNumber << ". - " << i << " Nedozvoljeni lable" << std::endl;
					exit(-1);
				}
				//mora da je u nekoj sekciji to proveriti
				if (curSection == "NULL")
				{
					std::cout << lineNumber << ". - " << i << " Nedozvoljena sekcija - NULL" << std::endl;
					exit(-1);
				}
				//ubaci u tabelu simbola
				//prvo skini ':'
				name = currentToken.getValue().substr(0, currentToken.getValue().size() - 1);
				labeldLine = true;
				if (symbolTable.insertSimbol(name, curSection, LABEL_SYMBOL, locationCounter, 0) == -1){
					std::cout << lineNumber << " ERROR"
							  << " OVO IME VEC U TABELI";
					exit(-1);
				}
				break;
			case Token_type::DIRECTIVE:
				//Ovo je prvi token ili drugi ako je prvi lable
				if (i != 0 + (labeldLine))
				{
					std::cout << "ERROR " << lineNumber << " DIREKTIVA NIJE NA POCETKU REDA" << std::endl;
				}
				//ako je .end zavrsi
				else if (currentToken.getValue() == ".end")
				{
					endFound = true;
					lineProcessed = true;
				}
				//ako je global pass
				else if (currentToken.getValue() == ".global")
				{
					//--Nista u prvom passu--
				}
				//ako je extern pass
				else if (currentToken.getValue() == ".extern")
				{
					//--- Ovo kada vidi kako radi linker
				}
				//ako je .secion:
				else if (currentToken.getValue() == ".section")
				{
					//jel dato ime sekciji i to pravilno
					currentToken = tokenizedLine[++i]; //gledamo sledeci token
					if (std::regex_match(currentToken.getValue(), symbolRegex) && tokenizedLine.size() == (2 + (labeldLine)))
					{
						//pohrani locationCounter trenutne sekcije
						for (auto &it : sectionTables)
						{
							if (it.getName() == curSection)
							{
								it.increasLocationCounter(locationCounter);
								break;
							}
						}
						//postaviti location counter na 0
						locationCounter = 0;
						//promeni trenutnu sekciju
						curSection = currentToken.getValue();
						//ubaci sekciju ako ne postoji vec u tabeli simbola i kreiraj njenu tabelu
						if (symbolTable.insertSimbol(curSection, curSection, SECTION_SYMBOL, 0) == -1)
						{
							std::cout << "ERROR " << lineNumber << " OVA SEKIJCA VEC POSTOJI" << std::endl;
							exit(-1);
						}
						//kreiraj tabelu
						sectionTables.push_back(SectionTable(curSection, 0));
					}
					else
					{
						std::cout << "ERROR " << lineNumber << " POGRESNA DEKLARACIJA SEKCIJE" << std::endl;
						exit(-1);
					}
				}
				//ako je .word:
				else if (currentToken.getValue() == ".word")
				{
					//izracunaj offset i uvecaj locationCounter
					//treba ovde proveriti zareze
					alocatedSpace = 0;
					for (int j = i + 1; j < tokenizedLine.size(); j++)
					{
						if (std::regex_match(tokenizedLine[j].getValue(), literalRegex))
						{
							alocatedSpace += 2;
						}
						else if(std::regex_match(tokenizedLine[j].getValue(), symbolRegex)){
							alocatedSpace += 2;
						}
						else
						{
							std::cout << "ERROR " << lineNumber << " POGRESNA .WORD DEKLARACIJA" << std::endl;
							exit(-1);
						}
					}
					//update sectio location counter
					locationCounter += alocatedSpace;
				}
				//ako je .skip:
				else if (currentToken.getValue() == ".skip")
				{
					//ispravni format
						if (std::regex_match(tokenizedLine[1 + (labeldLine)].getValue(), literalRegex) && tokenizedLine.size() == 2 + (labeldLine))
						{
							//izracunaj offset
							alocatedSpace = 0;
							currentToken = tokenizedLine[1 + labeldLine];
							if (std::regex_match(currentToken.getValue(), binaryRegex))
							{
								alocatedSpace = std::stoi(currentToken.getValue(), nullptr, 2);
							}
							else
							{
								alocatedSpace = std::stoi(currentToken.getValue(), nullptr, 0);
							}
							//proveri jel u opsegu 16bit
							if (alocatedSpace >= pow(2, 16) || alocatedSpace < (-(pow(2, 16))))
							{
								std::cout << "ERROR " << lineNumber << " ARGUMENT .SKIP VAN OPSEGA" << std::endl;
								exit(-1);
							}
							//update section location counter
							locationCounter += alocatedSpace;
							//initializatio in the section table is done in second pass
						}
						else{
							std::cout << "ERROR " << lineNumber << " NIJE LITERAL POSLE .SKIP" << std::endl;
							exit(-1);	
						}
				}
				//ako je .equ:
				else if (currentToken.getValue() == ".equ")
				{
					//dobar broj argumenata
					if (tokenizedLine.size() != 3 + (labeldLine))
					{
						std::cout << "ERROR " << lineNumber << " POGRESA BROJ ARGUMENATA ZA .EQU" << std::endl;
						exit(-1);
					}
					// || u && proveriti
					if (!std::regex_match(tokenizedLine[i + 1].getValue(), symbolRegex) || !std::regex_match(tokenizedLine[i + 2].getValue(), literalRegex))
					{
						std::cout << "ERROR " << lineNumber << " POGRESA TIP ARGUMENATA ZA .EQU" << std::endl;
						exit(-1);
					}
					//izracunaj operande
					if (std::regex_match(tokenizedLine[i + 2].getValue(), binaryRegex))
					{
						value = std::stoi(tokenizedLine[i + 2].getValue(), nullptr, 2);
					}
					else
					{
						value = std::stoi(tokenizedLine[i + 2].getValue(), nullptr, 0);
					}
					//ubaci u tabelu absolute sekcije
					absoluteSection.insertTableRow(absoluteSection.locationCounter, 2, toLittleEndianHex(value), createRowDescription(tokenizedLine[i+1].getValue(), value));
					//ubaci simbol ako je ovo prva definicija u tabelu simbola
					//location counter iz abs
					if (symbolTable.insertSimbol(tokenizedLine[i + 1].getValue(), "Aboslute", SYMBOL_SYMBOL, value, value) == -1)
					{
						std::cout << "ERROR " << lineNumber << " SIMBOL VEC POSTOJI ZA .EQU" << std::endl;
					}
					//promeni locationCounter
					absoluteSection.locationCounter+=2;
				}
				lineProcessed = true;
				break;
			case Token_type::INSTRUCTION:
				//mora da je u nekoj sekciji to proveriti
				if (curSection != "NULL")
				{
					//proveri jel dobar broj operatora
					//koliko ima operatora
					for (int j = 0; j < 27; j++)
					{
						if (instructionDescriptors[j].name == currentToken.getValue())
						{
							numOperands = instructionDescriptors[j].numOperators;
							instructionID = j;
							break;
						}
					}
					if (numOperands == tokenizedLine.size() - (labeldLine) - 1)
					{
						//jesu dobri operatori
						for (int j = 1; j <= numOperands; j++)
						{
							if (!tokenizedLine[i + j].getTokenType() == Token_type::OPERATOR)
							{
								std::cout << "ERROR " << lineNumber << "TOKEN " << j << " NIJE OPERATOR" << std::endl;
								exit(-1);
							}
						}
						//update section location counter

						// ako instrukcija ima vise nacina adresiranja
						//VRATI SE NA OVO
						if (instructionDescriptors[instructionID].minSize != instructionDescriptors[instructionID].maxSize)
						{
							if (tokenizedLine.size() - labeldLine == 2){
								name = tokenizedLine[labeldLine+1].getValue();
							}
							else{
								name = tokenizedLine[labeldLine+2].getValue();
							}
							if(name.at(0) == '*'){
								name.erase(0,1);
							}
							if(std::regex_match(name, registerRegex)){
								locationCounter += instructionDescriptors[instructionID].minSize;
							}
							else{
								locationCounter += instructionDescriptors[instructionID].maxSize;
							}
						}
						else
						{
							locationCounter += instructionDescriptors[instructionID].maxSize;
						}
					}
					else
					{
						std::cout << "ERROR " << lineNumber << " NEISPRAVAN BROJ OPERATOR ZA INSTRUKCIJU" << std::endl;
						exit(-1);
					}
				}
				else
				{
					std::cout << "ERROR " << lineNumber << " INSTRUKCIJA MORA BITI POZVANA U NEKOJ SEKCIJI" << std::endl;
					exit(-1);
				}
				lineProcessed = true;
				break;
			default:
				// <-- G R E S K A -->
				std::cout << "ERROR " << lineNumber << " NEOCEKIVAN TOKEN" << std::endl;
				lineProcessed = true;
				exit(-1);
				break;
			}
		}
	}
	//pohrani poslednju sekciju u tabelu i absolute
	if(endFound == false){
		std::cout << "NIJE NASAO .END DIREKTIVU";
	}
	sectionTables.back().increasLocationCounter(locationCounter);
	sectionTables.push_back(absoluteSection);

	//drugi prolaz
	lineNumber = 0;
	curSection = "NULL";
	endFound = false;
	
	//postavi kursor na pocetak
	input_file.seekg (0, input_file.beg);
	for(auto& tokenizedLine : tokenizedFile){
		//pomocne
		int symNumber;
		std::stringstream ss;
		std::stringstream ss2;
		std::string s;
		std::string instCode;
		std::string instName;
		std::string	firstRegCode;
		std::string secondRegCode;
		std::string addrMode;
		std::string instOffset;
		int instSize;
		bool pcRelative = false;
		bool immediate = false;
		bool direct = false;
		bool indirect = false;
		bool regOffset = false;
		
		//upravljacke
		bool lineProcessed = false;
		lineNumber++;
		bool labeldLine = false;
		for(int i = 0; i < tokenizedLine.size() && !lineProcessed && !endFound; i++){
			// for(auto it : tokenizedLine)
			// 	std::cout << it.getValue() << " ";
			// std::cout << "\n";
			Token currentToken = tokenizedLine[i];
			switch (currentToken.getTokenType())
			{
			case Token_type::LABEL :
				labeldLine = true;
				break;
			case Token_type::DIRECTIVE :
				if(currentToken.getValue() == ".extern"){
					
					i = i + 1;
					while(i < tokenizedLine.size()){
						if(std::regex_match(tokenizedLine[i].getValue(), symbolRegex)){
							symbolTable.insertSimbol(tokenizedLine[i].getValue(), "null", EXTERN_SYMBOL, -1, false);
						}
						else {
							std::cout << "ERROR " << lineNumber << " .EXTERN OCEKUJE SIMBOL TOKEN" << std::endl;
						}
						i++;
					}
				}
				else if(currentToken.getValue() == ".global"){
					//promeni flag u tabeli simbola
					i = i+1;
					while(i < tokenizedLine.size()){
						symbolTable.table.at(tokenizedLine[i++].getValue()).isLocal = false;
					}
				}
					// std::cout << (symbolTable.table.find(tokenizedLine[i+1].getValue())->second.isLocal ? "true" : "false") << std::endl;
				else if(currentToken.getValue() == ".section"){
					//promeni sekciju
					for(auto& it : sectionTables){
						if(it.getName() == curSection){
							it.increasLocationCounter(locationCounter);
							break;
						}
					}
					locationCounter = 0;
					curSection = tokenizedLine[i+1].getValue();
				}
				else if(currentToken.getValue() == ".end"){
					lineProcessed = true;
					endFound = true;
				}
				else if(currentToken.getValue() == ".word"){
					for(int j = 1 + labeldLine; j < tokenizedLine.size(); j++){
						if(std::regex_match(tokenizedLine[i+j].getValue(), symbolRegex)){
							//simbol postoji u tabeli simbola
							// std::cout << "DRUGI PROLAZ WORD" << std::endl;	
							if(symbolTable.table.find(tokenizedLine[i+j].getValue()) != symbolTable.table.end()){
								for(auto& it : sectionTables){
									if( it.getName() == curSection){
										//ako nije lokalni simbol
										if(symbolTable.table.find(tokenizedLine[i+j].getValue())->second.type != SYMBOL_SYMBOL){
											relocationTable.insertRow(1, tokenizedLine[i+j].getValue(), curSection, locationCounter);
											ss << ".word " <<  tokenizedLine[j].getValue() << " = " << 
												std::to_string(symbolTable.table.find(tokenizedLine[i+1].getValue())->second.offset) << std::endl;
											it.rows.push_back(SectionTableRow(
												locationCounter,
												2,
												"????",
												ss.str()
											));
										}
										//Ali ovaj simbol nikada ne pribapada nijednoj drugoj sekciji jer je on uvek u absolute
										//ako simbol lokalni
										else if(symbolTable.table.find(tokenizedLine[i+j].getValue())->second.type == SYMBOL_SYMBOL){
											ss << ".word " <<  tokenizedLine[j].getValue() << " = " << 
												std::to_string(symbolTable.table.find(tokenizedLine[i+1].getValue())->second.offset) << std::endl;
											ss2 << std::hex << symbolTable.table.find(tokenizedLine[i+1].getValue())->second.offset;
											it.rows.push_back(SectionTableRow(
												locationCounter,
												2,
												ss2.str(),
												ss.str()
											));
										}
										std::stringstream().swap(ss2);
										std::stringstream().swap(ss);
										break;
									}
								}
								locationCounter+= 2;
							}
							else{
								std::cout << tokenizedLine[i+j].getValue() << " SIMBOL NIJE U TABELI\n"; 
							}
						}
						else if(std::regex_match(tokenizedLine[i+j].getValue(), literalRegex)){
							alocatedSpace = 0;
							if (std::regex_match(tokenizedLine[i+j].getValue(), binaryRegex))
							{
								alocatedSpace = std::stoi(tokenizedLine[i+j].getValue(), nullptr, 2);
							}
							else
							{
								alocatedSpace = std::stoi(tokenizedLine[i+j].getValue(), nullptr, 0);
							}
							for(auto& it : sectionTables){
									if( it.getName() == curSection){
										ss << ".word " <<  alocatedSpace << '\n';
										it.rows.push_back(SectionTableRow(
											locationCounter,
											2,
											// toLittleEndianHex(symbolTable.table.find(tokenizedLine[i+1].getValue())->second.of),
											toLittleEndianHex(alocatedSpace),
											ss.str()
										));
										break;
									}
								}
							//update section location counter
							locationCounter+= 2;

						}
						else{
							std::cout << "ERROR " << lineNumber << " NEDOZVOLJENI OPERATOR ZA .WORD DIREKTIVU" << std::endl;
						}
					}

				}
				else if(currentToken.getValue() == ".skip"){
					//koliko bajtova ubacujem
					alocatedSpace = 0;
					if (std::regex_match(tokenizedLine[1+labeldLine].getValue(), binaryRegex))
					{
						alocatedSpace = std::stoi(tokenizedLine[1+labeldLine].getValue(), nullptr, 2);
					}
					else
					{
						alocatedSpace = std::stoi(tokenizedLine[1+labeldLine].getValue(), nullptr, 0);
					}
					//update section location counter
					locationCounter += alocatedSpace;
					ss << ".skip on line " << lineNumber << std::endl; 
					for(auto& it : sectionTables){
							if( it.getName() == curSection){
									for(int j = 0; j < alocatedSpace; j++){
										it.rows.push_back(SectionTableRow(
											locationCounter,
											1,
											"00",
											ss.str()
										));
									}
									break;
							}
					}
				}
				lineProcessed = true;
				break;
			case Token_type::INSTRUCTION :
				pcRelative = false;
				regOffset = false;
				indirect = false;
				direct = false;
				immediate = false;
				if(std::regex_match(currentToken.getValue(), noOperandInstruction)){
					instSize = 1;
					firstRegCode = "";
					secondRegCode = "";
					if(currentToken.getValue() == "halt"){
						//00
						instName = "halt\n";
						instCode = "00";
						}
					else if(currentToken.getValue() == "iret"){
						//20
						instName = "iret\n";
						instCode = "20";		
						}
					else if(currentToken.getValue() == "ret"){
						//40
						instName = "ret\n";
						instCode = "40";
					} 
				}
				else if(std::regex_match(currentToken.getValue(), singleRegInstruction)){
						instSize = 4;
						if(std::regex_match(tokenizedLine[i+1].getValue(), registerRegex)){
							firstRegCode = tokenizedLine[i+1].getValue().at(1);
						}
						else {
							std::cout << "ERROR: " << lineNumber << " NEDOZVOLJEN OPERAND " << std::endl; 
						}
						if(currentToken.getValue() == "int"){
							//10
							instName = "int\n";
							instCode = "10";
							secondRegCode = "F";
						}
						else if(currentToken.getValue() == "not"){
							//80
							instName = "not\n";
							instCode = "80";
							secondRegCode = firstRegCode;
						}
				}
				else if(std::regex_match(currentToken.getValue(), twoRegInstruction)){
						instSize = 2;
						if(std::regex_match(tokenizedLine[i+1].getValue(), registerRegex)){
							firstRegCode = tokenizedLine[i+1].getValue().at(1);
						}
						else{
							std::cout<<"ERROR " << lineNumber <<" LOS OPERATOR ZA twoRegInstruction - PRVI" << std::endl; 
						}
						if(std::regex_match(tokenizedLine[i+2].getValue(), registerRegex)){
							secondRegCode = tokenizedLine[i+2].getValue().at(1);
						}
						else{
							std::cout<<"ERROR " << lineNumber <<" LOS OPERATOR ZA twoRegInstruction - DRUGI" << std::endl; 
						}
						if(currentToken.getValue() == "add"){
							instName = "add";
							instCode = "D0";
						}
						else if(currentToken.getValue() == "sub"){
							instName = "sub";
							instCode = "D1";
						}
						else if(currentToken.getValue() == "mul"){
							instName = "mul";
							instCode = "D2";
						}
						else if(currentToken.getValue() == "div"){
							instName = "div";
							instCode = "D3";
						}
						else if(currentToken.getValue() == "cmp"){
							instName = "cmp";
							instCode = "D4";
						}
						else if(currentToken.getValue() == "and"){
							instName = "and";
							instCode = "81";
						}
						else if(currentToken.getValue() == "or"){
							instName = "or";
							instCode = "82";
						}
						else if(currentToken.getValue() == "xor"){
							instName = "xor";
							instCode = "83";
						}
						else if(currentToken.getValue() == "test"){
							instName = "test";
							instCode = "84";
						}
						else if(currentToken.getValue() == "shl"){
							instName = "shl";
							instCode = "90";
						}
						else if(currentToken.getValue() == "shr"){
							instName = "shr";
							instCode = "91";
						}
						instName.append(" ").append(tokenizedLine[i+1].getValue()).append(", ").append(tokenizedLine[i+2].getValue());
				}
				else if(std::regex_match(currentToken.getValue(), jumpInstruction)){
					//odredi ime i deskripor
					if(currentToken.getValue() == "jmp"){
						instCode = "50";
						instName = "jmp";
					}
					else if(currentToken.getValue() == "jeq"){
						instCode = "51";
						instName = "jeq";
					}
					else if(currentToken.getValue() == "jne"){
						instCode = "52";
						instName = "jne";
					}
					else if(currentToken.getValue() == "jgt"){
						instCode = "53";
						instName = "jgt";
					}
					else if(currentToken.getValue() == "call"){
						instCode = "30";
						instName = "call";
					}
					//namesti flegove
					if(tokenizedLine[i+1].getValue().at(0) == '%'){
						pcRelative = true;
						tokenizedLine[i+1].value.erase(0, 1);
					}
					if(tokenizedLine[i+1].getValue().at(0) == '*'){
						if(pcRelative == true){
							std::cout << "NEDOZVOLJEN NACIN ADRESIRANJA"<<endl
							break;
						}
						tokenizedLine[i+1].value.erase(0, 1);
						if(tokenizedLine[i+1].getValue().at(0) == '[' && tokenizedLine[i+1].getValue().back() == ']'){
							tokenizedLine[i+1].value.erase(0,1);
							tokenizedLine[i+1].value.pop_back();
							if(tokenizedLine[i+2].getValue().find('+') != std::string::npos)
								regOffset = true;
							else{
								indirect = true;
							}
						}
						else{
							direct = true;
						}
					}
					else{
						immediate = true;
					}
					//if(pcRelative && (regOffset || indirect || direct || immediate)){
					//	std::cout << "ERROR " << lineNumber << " NEDOZVOLJENI OPERATOR" << std::endl;
					//}
					//pc relativno adresiranj
					if(pcRelative){
						if(std::regex_match(tokenizedLine[i+1].getValue(), symbolRegex)){
								instSize = 5;
								firstRegCode = "7";
								secondRegCode = "F";
								//proveri ovo
								addrMode = "05";
							//simbol nije u tabeli simbola
							if(symbolTable.table.find(tokenizedLine[i+1].getValue()) == symbolTable.table.end()){
								std::cout << "ERROR " << lineNumber << " NEPOSTOJECI SIMBOL" << std::endl;
							}
							//simbol je u tabeli simbola
							instOffset = "????";
							instName.append(" pc relative linker");
							if(symbolTable.table.find(tokenizedLine[i+1].getValue())->second.section == curSection){
								std::stringstream().swap(ss);
								ss << std::hex << symbolTable.table.find(tokenizedLine[i+1].getValue())->second.offset - locationCounter;
								instOffset = ss.str();
								std::stringstream().swap(ss);
							}
							else{
								relocationTable.insertRow(R_386_PC16, tokenizedLine[i+1].getValue(), curSection, locationCounter+3);
							}
						}
						else{
							std::cout << "ERROR " << lineNumber << " NEDOZVOLJENI OPERATOR NAKON %" << std::endl;
						}
					}
					else if(immediate){
							instSize = 5;
							firstRegCode = "7";
							secondRegCode = "F";
							addrMode = "00";
						//ako literal
						if(std::regex_match(tokenizedLine[i+1].getValue(), literalRegex)){
							if (std::regex_match(tokenizedLine[i+1].getValue(), binaryRegex))
							{
								alocatedSpace = std::stoi(currentToken.getValue(), nullptr, 2);
							}
							else
							{
								alocatedSpace = std::stoi(tokenizedLine[i+1].getValue(), nullptr, 0);
							}
							//proveri jel u opsegu 16bit
							if (alocatedSpace >= pow(2, 16) || alocatedSpace < (-(pow(2, 16))))
							{
								std::cout << "ERROR " << lineNumber << " ARGUMENT INSTRUKCIJE VAN OPSEGA" << std::endl;
							}
							instOffset = toLittleEndianHex(alocatedSpace);
							instName.append(" literal");
						}
						//ako simbol
						else if(std::regex_match(tokenizedLine[i+1].getValue(), symbolRegex)){
								//van tabele
								if(symbolTable.table.find(tokenizedLine[i+1].getValue()) == symbolTable.table.end()){
									std::cout << "ERROR " << lineNumber << " NEPOSTOJECI SIMBOL" << std::endl;
								}
								//u tabeli
								instOffset = "????";
								if(symbolTable.table.find(tokenizedLine[i+1].getValue())->second.type == SYMBOL_SYMBOL){
									std::stringstream().swap(ss);
									ss << std::hex << symbolTable.table.find(tokenizedLine[i+1].getValue())->second.offset;
									instOffset = ss.str();
									std::stringstream().swap(ss);
								}
								else								
									relocationTable.insertRow(R_386_16, tokenizedLine[i+1].getValue(), curSection, locationCounter+3);
								
						}
						else{
							std::cout << "ERROR " << lineNumber << " NEDOZVOLJENI OPERAND" << std::endl;
						}
					}
					else if(direct){
						if(std::regex_match(tokenizedLine[i+1].getValue(), registerRegex)){
							instSize = 3;
							firstRegCode = tokenizedLine[i+1].getValue().at(tokenizedLine[i+1].getValue().size() - 1);
							secondRegCode = "F";
							addrMode = "01";
								firstRegCode = tokenizedLine[i+1].getValue().back();
								instName.append(" register direct");
						}
						else if(std::regex_match(tokenizedLine[i+1].getValue(), symbolRegex)){
							instSize = 5;
							firstRegCode = secondRegCode = "F";
							addrMode = "08";
							//van tabele
							if(symbolTable.table.find(tokenizedLine[i+1].getValue()) == symbolTable.table.end()){
								std::cout << "ERROR " << lineNumber << " NEPOSTOJECI SIMBOL" << std::endl;
							}
							//u tabeli
							instOffset = "????";
							if(symbolTable.table.find(tokenizedLine[i+1].getValue())->second.type == SYMBOL_SYMBOL){
								std::stringstream().swap(ss);
								ss << std::hex << symbolTable.table.find(tokenizedLine[i+1].getValue())->second.offset;
								instOffset = ss.str();
								std::stringstream().swap(ss);
							}
							else
								relocationTable.insertRow(R_386_16, tokenizedLine[i+1].getValue(), curSection, locationCounter+3);
						}
						else if(std::regex_match(tokenizedLine[i+1].getValue(), literalRegex)){
							instSize = 5;
							firstRegCode = secondRegCode = "F";
							addrMode = "08";
							if (std::regex_match(tokenizedLine[i+1].getValue(), binaryRegex))
							{
								alocatedSpace = std::stoi(currentToken.getValue(), nullptr, 2);
							}
							else
							{
								alocatedSpace = std::stoi(tokenizedLine[i+1].getValue(), nullptr, 0);
							}
							//proveri jel u opsegu 16bit
							if (alocatedSpace >= pow(2, 16) || alocatedSpace < (-(pow(2, 16))))
							{
								std::cout << "ERROR " << lineNumber << " ARGUMENT INSTRUKCIJE VAN OPSEGA" << std::endl;
							}	
							instOffset = toLittleEndianHex(alocatedSpace);
							instName.append(" direct literal");
						}
						else{
							std::cout << "ERROR " << lineNumber << " NEDOZVOLJENI OPERAND" << std::endl;
						}
					}
					else if(indirect){
						instSize = 3;
						secondRegCode = "F";
						addrMode = "02";
						if(std::regex_match(tokenizedLine[i+1].getValue(), registerRegex)){
							firstRegCode = tokenizedLine[i+1].getValue().back();
							instName.append(" indirect");
						}
						else{
							std::cout << "ERROR " << lineNumber << " NEDOZVOLJENI OPERAND" << std::endl;
						}
					}
					else if(regOffset){
						instSize = 5;
						firstRegCode = tokenizedLine[i+1].getValue().at(1);
						secondRegCode = "F";
						s = tokenizedLine[i+1].getValue().substr(tokenizedLine[i+1].getValue().find_first_of('+')+1 /*,tokenizedLine[i+1].getValue().find_first_of(']')-1*/);
						addrMode = "03";
						//simbol
						if(std::regex_match(s, symbolRegex)){
							//simbol van tabele
							if(symbolTable.table.find(s) == symbolTable.table.end()){
								std::cout << "ERROR " << lineNumber << " NEPOSTOJECI SIMBOL" << std::endl;
							}
							//simbol u tabel
								instOffset = "????";
								if(symbolTable.table.find(s)->second.type == SYMBOL_SYMBOL){
									std::stringstream().swap(ss);
									ss << std::hex << symbolTable.table.find(s)->second.offset;
									instOffset = ss.str();
									std::stringstream().swap(ss);
								}
								else
									relocationTable.insertRow(R_386_16, s, curSection, locationCounter+3);
						}
						//literal
						else if(std::regex_match(s, literalRegex)){
							if (std::regex_match(tokenizedLine[i+1].getValue(), binaryRegex))
							{
								alocatedSpace = std::stoi(s, nullptr, 2);
							}
							else
							{
								alocatedSpace = std::stoi(s, nullptr, 0);
							}
							//proveri jel u opsegu 16bit
							if (alocatedSpace >= pow(2, 16) || alocatedSpace < (-(pow(2, 16))))
							{
								std::cout << "ERROR " << lineNumber << " ARGUMENT INSTRUKCIJE VAN OPSEGA" << std::endl;
							}
							instOffset = toLittleEndianHex(alocatedSpace);
							instName.append(" offset with literal");
						}
						else{
							std::cout << "ERROR " << lineNumber << " NEDOZVOLJENI OPERAND" << std::endl;
						}
					}
				}
				else if(std::regex_match(currentToken.getValue(), ldStr)){
					if(currentToken.getValue() == "ldr"){
						instCode = "A0";
						instName = "ldr";
					}
					else if(currentToken.getValue() == "str"){
						instCode = "B0";
						instName = "str";
					}
					if(tokenizedLine[i+2].getValue().at(0) == '%'){
						pcRelative = true;
						tokenizedLine[i+2].value.erase(0, 1);
					}
					if(tokenizedLine[i+2].getValue().at(0) == '$'){
						immediate = true;
						tokenizedLine[i+2].value.erase(0, 1);
					}
					else if(tokenizedLine[i+2].getValue().at(0) == '[' && tokenizedLine[i+2].getValue().back() == ']'){
						tokenizedLine[i+2].value.erase(0,1);
						tokenizedLine[i+2].value.pop_back();
						if(tokenizedLine[i+2].getValue().find('+') != std::string::npos)
							regOffset = true;
						else{
							indirect = true;
						}
					}
					else{
						direct = true;
					}
					instSize = 5;
					if(std::regex_match(tokenizedLine[i+1].getValue(), registerRegex)){
						firstRegCode = tokenizedLine[i+1].getValue().back();
					}
					else{
						std::cout << "ERROR " << lineNumber << " NEDOZVOLJENI PRVI OPERAND" << std::endl;
					}
					secondRegCode = "F";
					if(pcRelative == true){
						if(std::regex_match(tokenizedLine[i+2].getValue(), symbolRegex)){
								secondRegCode = "7";
								addrMode = "03";
							//simbol nije u tabeli simbola
							if(symbolTable.table.find(tokenizedLine[i+2].getValue()) == symbolTable.table.end()){
								std::cout << "ERROR " << lineNumber << " NEPOSTOJECI SIMBOL" << std::endl;
							}
							//simbol je u tabeli simbola
							instOffset = "????";
							// instName.append("linker");
							if(curSection != symbolTable.table.find(tokenizedLine[i+2].getValue())->second.section)
								relocationTable.insertRow(R_386_PC16, tokenizedLine[i+2].getValue(), curSection, locationCounter+3);
							else{
								std::stringstream().swap(ss);
								ss << std::hex << symbolTable.table.find(tokenizedLine[i+2].getValue())->second.offset - locationCounter;
								instOffset = ss.str();
								std::stringstream().swap(ss);
							}
						}
						else{
							std::cout << "ERROR " << lineNumber << " NEDOZVOLJENI OPERATOR NAKON %" << std::endl;	
						}
					}
					else if(immediate == true){
						addrMode = "00";
						//ako literal
						if(std::regex_match(tokenizedLine[i+2].getValue(), literalRegex)){
							if (std::regex_match(tokenizedLine[i+2].getValue(), binaryRegex))
							{
								alocatedSpace = std::stoi(currentToken.getValue(), nullptr, 2);
							}
							else
							{
								alocatedSpace = std::stoi(tokenizedLine[i+2].getValue(), nullptr, 0);
							}
							//proveri jel u opsegu 16bit
							if (alocatedSpace >= pow(2, 16) || alocatedSpace < (-(pow(2, 16))))
							{
								std::cout << "ERROR " << lineNumber << " ARGUMENT INSTRUKCIJE VAN OPSEGA" << std::endl;
							}
							instOffset = toLittleEndianHex(alocatedSpace);
							instName.append(" literal");
						}
						//ako simbol
						else if(std::regex_match(tokenizedLine[i+2].getValue(), symbolRegex)){
								//van tabele
								if(symbolTable.table.find(tokenizedLine[i+2].getValue()) == symbolTable.table.end()){
									std::cout << "ERROR " << lineNumber << " NEPOSTOJECI SIMBOL" << std::endl;
								}
								//u tabeli
								instOffset = "????";
								if(symbolTable.table.find(tokenizedLine[i+2].getValue())->second.type == SYMBOL_SYMBOL){
									std::stringstream().swap(ss);
									ss << std::hex << symbolTable.table.find(tokenizedLine[i+2].getValue())->second.offset;
									instOffset = ss.str();
									std::stringstream().swap(ss);
								}
								else
									relocationTable.insertRow(R_386_16, tokenizedLine[i+2].getValue(), curSection, locationCounter+3);
						}
						else{
							std::cout << "ERROR " << lineNumber << " NEDOZVOLJENI DRUGI OPERAND" << std::endl;
						}
					}
					else if(direct == true){
						//ako registar
						if(std::regex_match(tokenizedLine[i+2].getValue(), registerRegex)){
							instSize = 3;
							addrMode = "01";
							secondRegCode = tokenizedLine[i+2].getValue().back();
							instName.append(" reg");
						}
						//ako literal
						else if(std::regex_match(tokenizedLine[i+2].getValue(), literalRegex)){
							addrMode = "04";
							if (std::regex_match(tokenizedLine[i+2].getValue(), binaryRegex))
							{
								alocatedSpace = std::stoi(currentToken.getValue(), nullptr, 2);
							}
							else
							{
								alocatedSpace = std::stoi(tokenizedLine[i+2].getValue(), nullptr, 0);
							}
							//proveri jel u opsegu 16bit
							if (alocatedSpace >= pow(2, 16) || alocatedSpace < (-(pow(2, 16))))
							{
								std::cout << "ERROR " << lineNumber << " ARGUMENT INSTRUKCIJE VAN OPSEGA" << std::endl;
							}
							instOffset = toLittleEndianHex(alocatedSpace);
							instName.append(" literal indirect");
						}
						//ako simbol
						else if(std::regex_match(tokenizedLine[i+2].getValue(), symbolRegex)){
								addrMode = "04";
								instName.append(" symbol indirect");
								//van tabele
								if(symbolTable.table.find(tokenizedLine[i+2].getValue()) == symbolTable.table.end()){
									std::cout << "ERROR " << lineNumber << " NEPOSTOJECI SIMBOL" << std::endl;
								}
								//u tabeli
								instOffset = "????";
								if(symbolTable.table.find(tokenizedLine[i+2].getValue())->second.type == SYMBOL_SYMBOL){
									std::stringstream().swap(ss);
									ss << std::hex << symbolTable.table.find(tokenizedLine[i+2].getValue())->second.offset;
									std::stringstream().swap(ss);
									instOffset = ss.str();
								}
								else
									relocationTable.insertRow(R_386_16, tokenizedLine[i+2].getValue(), curSection, locationCounter+3);
						}
						else{
							std::cout << "ERROR " << lineNumber << " NEDOZVOLJENI OPERAND" << std::endl;
						}

					}
					else if(indirect == true){
						if(std::regex_match(tokenizedLine[i+2].getValue(), registerRegex)){
							instSize = 3;
							addrMode = "02";
							secondRegCode = tokenizedLine[i+2].getValue().back();
							instName.append(" reg indirect");
						}
					}
					else if(regOffset == true){
						instSize = 5;
						secondRegCode = tokenizedLine[i+2].getValue().at(1);
						//simbol
						s = tokenizedLine[i+2].getValue().substr(tokenizedLine[i+2].getValue().find_first_of('+')+1);
						addrMode = "03";
						if(std::regex_match(s, symbolRegex)){
							//simbol van tabele
							if(symbolTable.table.find(s) != symbolTable.table.end()){
								std::cout << "ERROR " << lineNumber << " NEPOSTOJECI SIMBOL" << std::endl;
							}
							//simbol u tabeli
							instOffset = "????";
							if(symbolTable.table.find(s)->second.type == SYMBOL_SYMBOL){
								std::stringstream().swap(ss);
								ss << std::hex << symbolTable.table.find(s)->second.offset;
								instOffset = ss.str();
								std::stringstream().swap(ss);
							}
							else
								relocationTable.insertRow(R_386_16, s, curSection, locationCounter+3);
						}
						//literal
						else if(std::regex_match(s, literalRegex)){
							if (std::regex_match(tokenizedLine[i+2].getValue(), binaryRegex))
							{
								alocatedSpace = std::stoi(s, nullptr, 2);
							}
							else
							{
								alocatedSpace = std::stoi(s, nullptr, 0);
							}
							//proveri jel u opsegu 16bit
							if (alocatedSpace >= pow(2, 16) || alocatedSpace < (-(pow(2, 16))))
							{
								std::cout << "ERROR " << lineNumber << " ARGUMENT INSTRUKCIJE VAN OPSEGA" << std::endl;
							}
							instOffset = toLittleEndianHex(alocatedSpace);
							instName.append(" offset with literal");
						}
						else{
							std::cout << "ERROR " << lineNumber << " NEDOZVOLJENI OPERAND" << std::endl;
						}
					}
				}
				//push pop
				else {
					    // push <reg> => str <reg>, [sp] (sp -= 2 before);
    					// pop <reg> => ldr <reg>, [sp] (sp += 2 after);
					instSize = 3;
					if(currentToken.getValue() == "push"){
						instCode = "B0";
						addrMode = "12";
						firstRegCode = "8";
						secondRegCode = tokenizedLine[i+1].getValue().back();
						instName = "push";
					}
					else{
						instCode = "A0";
						addrMode = "42";
						firstRegCode = "8";
						secondRegCode = tokenizedLine[i+1].getValue().back();
						instName = "pop";
					}
				}
				instName.append("\n");
				std::stringstream().swap(ss);
				ss << instCode << firstRegCode << secondRegCode << addrMode << instOffset;
				s = ss.str();
				locationCounter += instSize;
					instName = "";
				for(int i = labeldLine; i < tokenizedLine.size(); i++ )
					instName.append(tokenizedLine[i].getValue()).append(" ");
				instName.append("\n");
				for(auto& it : sectionTables){
					if(it.getName() == curSection){
						it.insertTableRow(
							locationCounter,
							instSize,
							ss.str(),
							instName
						);
						break;
					}
				}
				break;		
			default:
				break;
			}
		}
	}
	for(auto& it : sectionTables){
		if(it.getName() == curSection)
			it.increasLocationCounter(locationCounter);
	}

	std::cout << "Ima - No. - Offset - isLocal - Value" << std::endl;
	symbolTable.printTable();
	std::cout << "-----------------------------------"<<std::endl;
	for(auto it : sectionTables)
		std::cout << it.printTable();
	std::cout << "Relocation table" << std::endl;
	for(auto it : relocationTable.rows){
		std::cout << it.no << " " << it.relocationType << " " << it.symbol << " " << it.section << " " << it.offset << std::endl;
	} 

	for(auto it: sectionTables){
		// txt_output_file << it.name << '\n';
		txt_output_file << it.getName() << '\n';
		for( auto iter : it.rows){
			txt_output_file << iter.bytecode;
		}
		txt_output_file<< '\n';
	}
	txt_output_file << "Relocation table" << '\n';
	for(auto it : relocationTable.rows){
		txt_output_file << it.no << " " << it.relocationType << " " << it.symbol << " " << it.section << " " << it.offset << '\n';
	}
	txt_output_file << "Symbol table" << '\n';
	for(auto it : symbolTable.table){
		txt_output_file << it.second.no << " " << it.first << " " << it.second.section << " " << it.second.offset << " " << it.second.value << " " << it.second.isLocal << '\n'; 
	}

	input_file.close();
	txt_output_file.close();
	return 0;
}
