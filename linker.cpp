
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <regex>
#include "tables.hpp"
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

SymbolTable getSymbolTableFromFile(std::string input){
    int no;
    std::string section;
    int offset;
    bool isLocal;
    int value;
    std::string name;
    std::ifstream input_file;
    input_file.open(input, std::ifstream::in);
    if (!input_file.is_open())
		std::cout << "Greska pri otvaranj inputa" << std::endl;
    
    SymbolTable symbolTable;
    std::string line;
    bool read = false;;
    while (std::getline(input_file, line)){
        if(line == "Symbol table"){
            read = true;
        }
        else{
            if(read){
                std::stringstream ss;
                ss << line;
                std::getline(ss, line, ' ');
                no = std::stoi(line);
                std::getline(ss, line, ' ');
                name = line;
                std::getline(ss, line, ' ');
                section = line;
                std::getline(ss, line, ' ');
                offset = std::stoi(line);
                std::getline(ss, line, ' ');
                value = std::stoi(line);
                std::getline(ss, line, ' ');
                isLocal = line == "1";
                
                std::pair<std::string, SymbolTableRow> row{name, SymbolTableRow(no, section, offset,isLocal, value)};
                symbolTable.table.insert(row);
            }
        }
    }
    input_file.close();
    return symbolTable;
}

RelocationTable getRelocationTableFromFile(std::string input){
    int no = 0;
    int relocationType = -1;
    std::string symbol = "";
    std::string section = " ";
    int offset = -1;
    std::ifstream input_file;
    input_file.open(input, std::ifstream::in);
    if (!input_file.is_open())
		std::cout << "Greska pri otvaranj inputa" << std::endl;
    
    RelocationTable relocationTable;
    std::string line;
    bool read = false;;
    while (std::getline(input_file, line)){
        if(line == "Relocation table"){
            read = true;
        }
        else if(line == "Symbol table")
            read = false;
        else{
            if(read){
                std::stringstream ss;
                ss << line;
                std::getline(ss, line, ' ');
                no = std::stoi(line);
                std::getline(ss, line, ' ');
                relocationType = std::stoi(line);
                std::getline(ss, line, ' ');
                symbol = line;
                std::getline(ss, line, ' ');
                section = line; 
                std::getline(ss, line, ' ');
                offset = std::stoi(line);
                relocationTable.rows.push_back(RelocationTableRow(no,relocationType, symbol, section, offset));
            }
        }
    }
    input_file.close();
    return relocationTable;
}

std::vector<SectionTable> getSectionTableFromFile(std::string input){
    std::string name;
    std::string byteCode;

    std::ifstream input_file;
    input_file.open(input, std::ifstream::in);
    if (!input_file.is_open())
		std::cout << "Greska pri otvaranj inputa" << std::endl;
    
    std::vector<SectionTable> sectionTable;
    std::string line;
    bool read = true;;
    while (std::getline(input_file, line)){
        if(line == "Relocation table"){
            read = false;
            break;
        }
        else{
            if(read){
                name = line;
                std::getline(input_file, line);
                byteCode = line;
                SectionTable section(name, line.size()/2);
                section.insertTableRow(0, byteCode.size()/2, byteCode, "");
                sectionTable.push_back(section);
               }
        }
    }
    input_file.close();
    return sectionTable;
}

class PlaceSection {
public:
    std::string name;
    int location;
    PlaceSection(std::string name, int location) :
    name{name},
    location{location}
    {}
};
 
class ObjectFile{
public:
    std::string name;
    SymbolTable symbolTable;
    RelocationTable relocationTable;
    std::vector<SectionTable> sectionTable;


    ObjectFile(std::string input){
        name = input;
        symbolTable = getSymbolTableFromFile(input);
        relocationTable = getRelocationTableFromFile(input);
        sectionTable = getSectionTableFromFile(input);
    }
};




int main(int argc, char *argv[]){
    std::vector<SectionTable> sectionTable;
    SymbolTable symbolTable;
    std::vector<PlaceSection> placeSection;
    std::vector<std::string> inputFiles;
    std::vector<ObjectFile> objectFiles;
    std::regex placeRegex("^-place=\\.{0,1}[a-zA-Z_][a-zA-Z0-9_]*@0x[0-9a-fA-F]{1,4}$");
    bool files = false;
    for(int i = 1; i < argc; i++){
        std::string input = argv[i];
        if(std::regex_match(input, placeRegex)){
            std::string sectionName = input.substr(input.find('=') + 1, input.find('@') - input.find('=') - 1);
            int location = std::stoi(input.substr(input.find('@') + 1, input.size() - input.find('@')).c_str(), 0, 16);
            placeSection.push_back(PlaceSection(sectionName, location));
        }
        else if(input == "-o"){
            files = true;
        }
        else if(files)
            inputFiles.push_back(input);
    }

    for(std::string it : inputFiles){
        objectFiles.push_back(ObjectFile(it));
    }
    //spoji tabele i proemni offsete
    for(auto& it : objectFiles){
        // prodji kroz svaku sekcije
        for(auto iter : it.sectionTable){
            bool sectionIn = false;
            //da li je vec ucitana
            for(auto hlp : sectionTable){
                if(hlp.name == iter.name){
                    sectionIn = true;
                    break;
                }
            }
            //ako ucitana
            if(sectionIn){
                //prodje kroz sve simbole iz tog fajla
                for(auto& symbol : it.symbolTable.table){
                    // ako simbol label 
                    if(symbol.second.section != "Aboslute" && symbol.second.section != "null"){
                        int sectionOffset = 0;
                        //nadji velicinu svih sekcija pre sekcije ovog simbola
                        for(auto hlp : sectionTable){
                            if(hlp.name == symbol.second.section){
                                sectionOffset = hlp.locationCounter;
                                break;
                            }
                        }
                        if(symbol.second.section == iter.name)
                            symbol.second.offset += sectionOffset;
                    } 
                }
                // prodji kroz relakocionu tabelu
                for(auto& relocation : it.relocationTable.rows){
                    int sectionOffset = 0;
                    //pronadji sekciju u linker tabeli
                    for(auto hlp : sectionTable){
                        if(hlp.name == relocation.section){
                            sectionOffset = hlp.locationCounter;
                            break;
                        }
                    }
                    //povecaj za dosadasnju velicinu
                    if(relocation.section == iter.name)
                        relocation.offset += sectionOffset;
                }
                for(auto& hlp : sectionTable){
                    if(hlp.name == iter.name){
                        hlp.rows.push_back(SectionTableRow(hlp.locationCounter, iter.locationCounter, iter.rows[0].bytecode, it.name));                      
                        hlp.locationCounter += iter.locationCounter;
                        break;
                    }
                }
            }
            else{
                SectionTable newSection(iter.name, iter.locationCounter);
                newSection.rows.push_back(SectionTableRow(0, iter.locationCounter, iter.rows[0].bytecode, it.name));
                sectionTable.push_back(newSection);
            }
        }
    }
    //spoji simbole
    for(auto it : objectFiles){
        for(auto iter : it.symbolTable.table){
            //ako postoji i nije extern
            if(symbolTable.table.find(iter.first) == symbolTable.table.end() && iter.second.offset != -1){
                symbolTable.insertSimbol(iter.first, iter.second.section, iter.second.offset, iter.second.value, iter.second.isLocal);                
            }
            // else{
                //a sta ako sekcija
                // std::cout << "Simbol " << iter.first << " je visestruko definisan" << std::endl;
            // }
        }
    }
    //proveri da li dolazi do preklapanja
    for(auto it : placeSection){
        for(auto iter : sectionTable){
            if(it.name == iter.name){
                int endAddress = it.location + iter.locationCounter;
                for(auto size : placeSection){
                    //ako je smestena pre kraja i posle pocetka neke drug sekcije
                    if(endAddress > size.location && it.location <= size.location && size.name != iter.name)
                        std::cout << "Sekcija " << size.name << " se preklapa sa sekcijom " <<  iter.name << std::endl;
                }
            break;
            }
        }
    }
    //spoji u output
    for(auto it : objectFiles){
        for(auto iter : it.relocationTable.rows){
            if(symbolTable.table.find(iter.symbol) != symbolTable.table.end()){
                SymbolTableRow symbol = symbolTable.table.find(iter.symbol)->second;
                bool symbolFromSameFile = false;
                for(auto hlp : it.symbolTable.table){
                    if(hlp.first == iter.symbol){
                        symbolFromSameFile = true;
                        break;
                    }
                }
                if((symbol.isLocal == true && (symbol.section != iter.section || symbolFromSameFile)) || symbol.isLocal == false){
                    //R_386_16 1
                    if(iter.relocationType == 1){
                        std::string writeValue = toLittleEndianHex(symbol.offset);
                        for(auto& section : sectionTable){
                            //ako u ovoj sekciji
                            if(section.name == iter.section){
                                int count = 0;
                                //nadji gde da upises
                                for(auto& row : section.rows){
                                    count += row.size;
                                    if(count >= iter.offset){
                                        count -= row.size;
                                        count = iter.offset - count;
                                        //upisujemo u count
                                        row.bytecode.replace(count*2, 4, writeValue);
                                    }
                                }
                                break;
                            }
                        }
                    }
                    //ovo verovatno treba drugacije da se odradi
                    else{
                        std::string writeValue = toLittleEndianHex(symbol.offset);
                        for(auto& section : sectionTable){
                            //ako u ovoj sekciji
                            if(section.name == iter.section){
                                int count = 0;
                                //nadji gde da upises
                                for(auto& row : section.rows){
                                    count += row.size;
                                    if(count >= iter.offset){
                                        count -= row.size;
                                        count = iter.offset - count;
                                        //upisujemo u count
                                        row.bytecode.replace(count*2, 4, writeValue);
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
            else{
                std::cout << "Ne moze da poveze " << iter.symbol << " sa " << iter.no << " relokacionim zapisom iz " << it.name << std::endl;
            }
        }
    }

    // getSymbolTableFromFile(argv[1]).printTable();
    // RelocationTable rt = getRelocationTableFromFile(argv[1]);
    // for(auto it : rt.rows){
    //     std::cout << it.no << " " << it.relocationType << " " << it.numSymbol << " " << it.section << " " << it.offset << '\n';
    // }
    // std::vector<SectionTable> sectionTable = getSectionTableFromFile(argv[1]);
    // for(auto it : sectionTable){    
    //     std::cout << "." << it.name << " " << it.locationCounter;
    //     std::cout << std::endl;
    //     for(int i = 0; i < (it.locationCounter*2)/16; i++){
    //         for(int j = 0; j < 16; j++)
    //             std::cout << it.rows[0].bytecode.at(i*16+j);
    //         std::cout << std::endl;
    //     }
    //     for(int i = 0; i < (it.locationCounter*2)%16; i++)
    //         std::cout << it.rows[0].bytecode.at(it.locationCounter - (it.locationCounter%16)     + i);
    //     std::cout << std::endl;
    // }

    // for(auto it : sectionTable){
    //     std::cout << it.getName() << " " << it.locationCounter << '\n';
    //     for(auto iter : it.rows){
    //         std::cout << iter.description << " " << iter.offset << "-" << iter.size << std::endl;
    //         std::cout << iter.bytecode << std::endl;
    //     }
    // }
    // for(auto it : objectFiles){
    //     std::cout << it.name << std::endl;
    //     for(auto iter : it.relocationTable.rows)
    //         std::cout << iter.no << " " << iter.symbol << " " << iter.section << " " << iter.offset << std::endl; 
    // }
    // symbolTable.printTable();
    // for(auto it : placeSection){
    //     std::cout << it.name << " " << it.location << std::endl;
    // }

}