#include <string>
#include <map>
#include <utility>
#include <vector>

#include "tables.hpp"

SymbolTableRow::SymbolTableRow(int no, std::string section,int type, int offset, bool isLocal, int value):
        no{no},
        section{section},
        type{type},
        offset{offset},
        isLocal{isLocal},
        value{value}
    {}

std::string SymbolTableRow::printSymbol(){
    std::string ret;
    ret.append(std::to_string(no));
    ret.append(" ");
    ret.append(section);
    ret.append(" ");
    ret.append(std::to_string(offset));
    ret.append(" ");
    ret.append(std::to_string(isLocal));
    ret.append(" ");
    ret.append(std::to_string(value));
    return ret;
}


int SymbolTable::insertSimbol(std::string name, std::string section,int type, int offset, int value = 0, bool isLocal = true){
        //proveri jel sekcija nije noSection; ovo sam uradio u pozivu
        if(table.find(name) == table.end()){
            std::pair<std::string, SymbolTableRow> 
            row{name, SymbolTableRow(++COUNTER,section,type,offset, isLocal, value)};
            table.insert(row);
            return 1;
        }
        else{
            std::cout << "SIMBOL VEC POSTOJI" << std::endl;
            return -1;
        }
}
void SymbolTable::printTable(){
    for(auto& elem : table){
        std::cout << elem.first << " " << elem.second.printSymbol() << std::endl;
    }
}

SectionTableRowl::SectionTableRow(int offset, int size, std::string bytecode, std::string description) :
    offset{offset},
    size{size},
    bytecode{bytecode},
    description{description}
    {}
std::string SectionTableRowl::printRow(){
    std::string ret;
    for(int i = 0; i < bytecode.size(); i += 2){
        ret.append(bytecode.substr(i, 1));
        ret.append(bytecode.substr(i + 1, 1));
        ret.append(" ");
    }
    for(int i = 0; i < 5 - size; i++){
        ret.append("   ");
    }
    ret.append("\t");
    ret.append(description);
    return ret;
}
SectionTable::SectionTable(std::string name, int locationCounter) :
    name{name},
    locationCounter{locationCounter}
    {}
void SectionTable::increasLocationCounter(int inc){
    locationCounter = inc;
}
void SectionTable::insertTableRow(int offset, int size, std::string bytecode, std::string description){
    rows.push_back(SectionTableRow(locationCounter,size,bytecode,description ));
}
std::string SectionTable::getName(){
    return name;
}

std::string SectionTable::printTable(){
    int num = 0;     
    std::string ret;
    ret.append(name);
    ret.append(" ");
    ret.append(std::to_string(locationCounter));
    ret.append("\n");
    for(auto it : rows){
        ret.append(std::to_string(num).append("\t")).append(it.printRow());
        num+=it.size;
    }
    return ret;
}

RelocationTableRow::RelocationTableRow(int no, int relocationType, std::string symbol, std::string section, int offset) :
    no{no},
    relocationType{relocationType},
    symbol{symbol},
    section{section},
    offset{offset}
    {}

void RelocationTable::insertRow(int relocationType, std::string symbol, std::string section, int offset){
    rows.push_back(RelocationTableRow(idNum++, relocationType, symbol, section, offset));
}
