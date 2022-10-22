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
    SymbolTableRow(int no, std::string section,int type, int offset, bool isLocal, int value);
    std::string printSymbol();
};
class SymbolTable{
public:
    static int COUNTER;
    std::map<std::string, SymbolTableRow> table;

    int insertSimbol(std::string name, std::string section,int type, int offset, int value = 0, bool isLocal = true);
    void printTable();
};
int SymbolTable::COUNTER = 0;
//izmeni malo ovo
class SectionTableRow {
public:
    int offset;
    int size;
    std::string bytecode;
    std::string description;

    SectionTableRow(int offset, int size, std::string bytecode, std::string description);
    std::string printRow();
};


class SectionTable{
public:
    std::string name;
    int locationCounter;
    std::vector<SectionTableRow> rows;

    SectionTable(std::string name, int locationCounter);
    void increasLocationCounter(int inc);

    void insertTableRow(int offset, int size, std::string bytecode, std::string description);

    std::string getName();
    std::string printTable();
};

class RelocationTableRow{
public:
    int no;
    int relocationType;
    std::string symbol;
    std::string section;
    int offset;

    RelocationTableRow(int no, int relocationType, std::string symbol, std::string section, int offset);
};
class RelocationTable{
public:
    std::vector<RelocationTableRow> rows;
    int idNum = 1;
    void insertRow(int relocationType, std::string symbol, std::string section, int offset);
