#include <fstream>

#include <allegro5/allegro.h>

#include "data_file.h"

using namespace std;

//Creates a dummy node. If the programmer requests an invalid node, a dummy is returned.
data_node* data_node::create_dummy() {
    data_node* new_dummy_child = new data_node();
    dummy_children.push_back(new_dummy_child);
    return new_dummy_child;
}

//Returns the number of children nodes (direct children only).
size_t data_node::get_nr_of_children() {
    return children.size();
}

//Returns a child node given its number on the list (direct children only).
data_node* data_node::get_child(size_t number) {
    if(number >= children.size()) return create_dummy();
    return children[number];
}

//Returns the number of occurences of a child name (direct children only).
size_t data_node::get_nr_of_children_by_name(string name) {
    size_t number = 0;
    
    for(size_t c = 0; c < children.size(); c++) {
        if(name == children[c]->name) number++;
    }
    
    return number;
}

//Returns the nth child with this name on the list (direct children only).
data_node* data_node::get_child_by_name(string name, size_t occurrence_number) {
    size_t cur_occurrence_number = 0;
    
    for(size_t c = 0; c < children.size(); c++) {
        if(name == children[c]->name) {
            if(cur_occurrence_number == occurrence_number) {
                //We found it.
                return children[c];
            } else {
                cur_occurrence_number++;
            }
        }
    }
    
    return create_dummy();
}

//Returns the value of a node, or def if it has no value.
string data_node::get_value_or_default(string def) {
    return (value.size() == 0 ? def : value);
}

//Adds a new child to the list.
size_t data_node::add(data_node* new_node) {
    children.push_back(new_node);
    return children.size() - 1;
}

//Removes and destroys a child from the list.
bool data_node::remove(data_node* node_to_remove) {
    for(size_t c = 0; c < children.size(); c++) {
        if(children[c] == node_to_remove) {
            delete node_to_remove;
            children.erase(children.begin() + c);
            return true;
        }
    }
    return false;
}

//Loads data from a file.
void data_node::load_file(string filename, bool trim_values) {
    vector<string> lines;
    
    bool is_first_line = true;
    file_was_opened = false;
    this->filename = filename;
    
    ALLEGRO_FILE* file = al_fopen(filename.c_str(), "r");
    if(file) {
        file_was_opened = true;
        while(!al_feof(file)) {
            string line;
            getline(file, line);
            
            if(is_first_line) {
                //Let's just check if it starts with the UTF-8 Magic Number.
                if(line.size() >= 3) {
                    if(line.substr(0, 3) == UTF8_MAGIC_NUMBER) line = line.erase(0, 3);
                }
            }
            lines.push_back(line);
            is_first_line = false;
        }
        al_fclose(file);
    }
    
    load_node(lines, trim_values, 0);
}

//Loads data from a list of text lines.
//Returns the number of the line this node ended on, judging by start_line. This is used for the recursion.
size_t data_node::load_node(vector<string> &lines, bool trim_values, size_t start_line) {
    children.clear();
    
    if(start_line > lines.size()) return start_line;
    
    for(size_t l = start_line; l < lines.size(); l++) {
        string line = lines[l];
        
        line = trim_spaces(line, true);     //Removes the leftmost spaces.
        
        if(line.size() >= 2) {
            if(line[0] == '/' && line[1] == '/') continue;  //A comment, ignore this line.
        }
        
        //Option=value
        size_t pos = line.find('=');
        if(pos != string::npos && pos > 0 && line.size() >= 2) {
        
            string v = line.substr(pos + 1, line.size() - (pos + 1));
            if(trim_values) v = trim_spaces(v);
            
            data_node* new_child = new data_node();
            new_child->name = trim_spaces(line.substr(0, pos));
            new_child->value = v;
            new_child->file_was_opened = file_was_opened;
            new_child->filename = filename;
            new_child->line_nr = l;
            children.push_back(new_child);
            
            continue;
        }
        
        //Sub-node start
        pos = line.find('{');
        if(pos != string::npos && pos > 0 && line.size() >= 2) {
        
            size_t new_child_line = l;
            
            data_node* new_child = new data_node();
            l = new_child->load_node(lines, trim_values, l + 1);
            new_child->name = trim_spaces(line.substr(0, pos));
            new_child->value = "";
            new_child->file_was_opened = file_was_opened;
            new_child->filename = filename;
            new_child->line_nr = new_child_line;
            children.push_back(new_child);
            
            continue;
        }
        
        //Sub-node end
        pos = line.find('}');
        if(pos != string::npos) {
            return l;
        }
    }
    
    return lines.size() - 1;
}

//Creates an empty data node.
data_node::data_node() {
    file_was_opened = false;
    line_nr = 0;
}

//Creates a data node, using the data and creating a copy of the children from another node.
data_node::data_node(const data_node &dn2) {
    for(size_t c = 0; c < dn2.children.size(); c++) {
        children.push_back(new data_node(*(dn2.children[c])));
    }
    for(size_t dc = 0; dc < dn2.dummy_children.size(); dc++) {
        dummy_children.push_back(new data_node(*(dn2.dummy_children[dc])));
    }
    
    name = dn2.name;
    value = dn2.value;
    file_was_opened = dn2.file_was_opened;
    filename = dn2.filename;
    line_nr = dn2.line_nr;
}

//Creates a data node from a file, given the file name.
data_node::data_node(string filename, bool trim_values) {
    file_was_opened = false;
    line_nr = 0;
    load_file(filename, trim_values);
}

//Destroys a data node and all the children within.
data_node::~data_node() {
    for(size_t c = 0; c < children.size(); c++) {
        delete children[c];
    }
    
    for(size_t dc = 0; dc < dummy_children.size(); dc++) {
        delete dummy_children[dc];
    }
}

/* ----------------------------------------------------------------------------
 * Like an std::getline(), but for ALLEGRO_FILE*.
 */
void getline(ALLEGRO_FILE* file, string &line) {
    line = "";
    if(!file) {
        return;
    }
    
    size_t bytes_read;
    char* c_ptr = new char;
    char c;
    
    bytes_read = al_fread(file, c_ptr, 1);
    while(bytes_read > 0) {
        c = *((char*) c_ptr);
        
        if(c == '\r' || c == '\n') {
            break;
        } else {
            line.push_back(c);
        }
        
        bytes_read = al_fread(file, c_ptr, 1);
    }
    
    delete c_ptr;
}

/* ----------------------------------------------------------------------------
 * Removes all trailing and preceding spaces.
 * This means space and tab characters before and after the 'middle' characters.
 * s:         The original string.
 * left_only: If true, only trim the spaces at the left.
 */
string trim_spaces(string s, bool left_only) {
    //Spaces before.
    if(s.size()) {
        while(s[0] == ' ' || s[0] == '\t') {
            s.erase(0, 1);
            if(s.size() == 0) break;
        }
    }
    
    if(!left_only) {
        //Spaces after.
        if(s.size()) {
            while(s[s.size() - 1] == ' ' || s[s.size() - 1] == '\t') {
                s.erase(s.size() - 1, 1);
                if(s.size() == 0) break;
            }
        }
    }
    
    return s;
}
