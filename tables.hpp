#include <string>
#include <map>
#include <utility>
#include <vector>
class SymbolTableRow{
public:
    int no;
    std::string section;
    int type;
    int offset;
    bool isLocal;
    int value;
    //nesto za relokacionu tabelu;
    SymbolTableRow(int no, std::string section,int type, int offset, bool isLocal, int value):
        no{no},
        section{section},
        type{type},
        offset{offset},
        isLocal{isLocal},
        value{value}
    {}
    std::string printSymbol(){
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
};
class SymbolTable{
public:
    static int COUNTER;
    std::map<std::string, SymbolTableRow> table;

    int insertSimbol(std::string name, std::string section,int type, int offset, int value = 0, bool isLocal = true){
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

    void printTable(){
        for(auto& elem : table){
            std::cout << elem.first << " " << elem.second.printSymbol() << std::endl;
        }
    }

};
int SymbolTable::COUNTER = 0;
//izmeni malo ovo
class SectionTableRow {
public:
    int offset;
    int size;
    std::string bytecode;
    std::string description;

    SectionTableRow(int offset, int size, std::string bytecode, std::string description) :
        offset{offset},
        size{size},
        bytecode{bytecode},
        description{description}
        {}
    
    std::string printRow(){
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
};


class SectionTable{
public:
    std::string name;
    int locationCounter;
    std::vector<SectionTableRow> rows;

    SectionTable(std::string name, int locationCounter) :
        name{name},
        locationCounter{locationCounter}
    {}
    void increasLocationCounter(int inc){
        locationCounter = inc;
    }

    void insertTableRow(int offset, int size, std::string bytecode, std::string description){
        rows.push_back(SectionTableRow(locationCounter,size,bytecode,description ));
    }

    std::string getName(){
        return name;
    }

    std::string printTable(){
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
};
class RelocationTableRow{
public:
    int no;
    int relocationType;
    std::string symbol;
    std::string section;
    int offset;

    RelocationTableRow(int no, int relocationType, std::string symbol, std::string section, int offset) :
        no{no},
        relocationType{relocationType},
        symbol{symbol},
        section{section},
        offset{offset}
        {}
};
class RelocationTable{
public:
    std::vector<RelocationTableRow> rows;
    int idNum = 1;
    void insertRow(int relocationType, std::string symbol, std::string section, int offset){
        rows.push_back(RelocationTableRow(idNum++, relocationType, symbol, section, offset));
    }
};
