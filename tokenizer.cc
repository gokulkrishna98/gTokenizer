#include "tokenizer.h"
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <unordered_map>


struct Utftext {
	std::unordered_map<uint32_t, std::vector<uint32_t>> decomposition_map;
	std::vector<uint8_t> file_content;

	std::vector<uint32_t> _nfd_transformation(uint32_t ch);
	uint32_t _to_lowercase(uint32_t ch);
	void _parse_decomposition_map(const char* filename);
	void _print_decomposition_map();

	Utftext(const char *filename, bool parse_normalization_data = true); 
	uint32_t iterate();
	std::vector<uint8_t> convert2utf_8(uint32_t val);
	std::vector<uint32_t> normalize_char(uint32_t ch);
	void normalize();
	bool is_end();
	size_t index = 0;
	void write_back(const char* filename);
};


void Utftext::_print_decomposition_map(){
	printf("printing decomposition_map .... \n");
	printf("[codepoint] -> [decomposition codepoints]\n");
	for(auto it: decomposition_map){
		printf("[%x] -> ", it.first);
		printf("[ ");
		for(auto dv: it.second)
			printf("%x ", dv);
		printf("]\n");
	}
	return;	
}

/*
It is a helper function that populates the decomposition map
- Parse the UnicodeData.txt (from : https://www.unicode.org/Public/14.0.0/ucd/UnicodeData.txt)
  line by line.
- Ignore the decomposition which has <compat> tag, as they belong to 
  compatiblity normalization (nfkc/nfkd).
- populate the decomposition map which maps to nfd normalization.
*/
void Utftext::_parse_decomposition_map(const char* filename){
	std::ifstream file(filename);
	std::string line;

	if(!file.is_open()){
		fprintf(stderr, "Error opening UnicodeData.txt file..");
		exit(1);
	}

	auto is_hexadecimal = [](std::string str) -> bool {
		for (size_t i = 0; i < str.length(); ++i) {
			if (!std::isxdigit(static_cast<uint8_t>(str[i]))) {
            	return false;
        	}
    	}
    	return true;
	};

	while(std::getline(file, line)){
		std::istringstream stream(line);	
		std::string token;
		std::vector<std::string> fields;
		while(std::getline(stream, token, ';')) {
			fields.push_back(token);	
		}
		uint32_t code_point = std::stoull(fields[0], nullptr, 16);
		if(!fields[5].empty()){
			std::istringstream decomp_rule(fields[5]);	
			std::vector<uint32_t> decomposition;
			while(std::getline(decomp_rule, token, ' ')){
				if(!is_hexadecimal(token)) break;	
				decomposition.push_back(std::stoull(token, nullptr, 16));
			}
			if(!decomposition.empty()){
				decomposition_map[code_point] = decomposition;
			}
		}
	}
	return;	
}

std::vector<uint32_t> Utftext::_nfd_transformation(uint32_t ch){
	if(decomposition_map.find(ch) != decomposition_map.end()){
		return decomposition_map[ch];
	}
	return {ch};	
}

uint32_t Utftext::_to_lowercase(uint32_t ch){
	if(ch >= 0x41 && ch <= 0x5a){
		return 0x61 + (ch - 0x41);
	}
	return ch;	
}

Utftext::Utftext(const char * filename, bool parse_normalization_data){
	std::ifstream file(filename, std::ios::binary);
	if(!file.is_open()){
		fprintf(stderr, "Error opening file\n");
		exit(1);
	}
	char byte;
	while (file.get(byte)) {
	    file_content.push_back(static_cast<uint8_t>(byte));
	}	
	file.close();

	if(parse_normalization_data){
		_parse_decomposition_map("UnicodeData.txt");
	}
}

void Utftext::write_back(const char* filename){
	std::ofstream outfile(filename, std::ios::binary);
	if (!outfile) {
		fprintf(stderr, "Error writing to the write back file");
	    exit(1);
	}
	outfile.write(reinterpret_cast<const char*>(file_content.data()), file_content.size());
	outfile.close();
}

bool Utftext::is_end(){
	if(index >= file_content.size()){
		return true;
	}	
	return false;
}

void Utftext::normalize(){
	std::vector<uint8_t> new_file_content;
	while(!is_end()){
		uint32_t ch = iterate();
		std::vector<uint32_t> nc = normalize_char(ch);
		if(!nc.empty()){
			std::vector<uint8_t> uc = convert2utf_8(nc[0]);
			new_file_content.insert(new_file_content.end(), uc.begin(), uc.end());
		}
	}
	index = 0;
	file_content = new_file_content;
	return;
}

/*
If the i is valid start index, then it parses the utf codepoint value,
incase it is not valid then we return -1 i.e 0xffff'ffff (which is not used);
*/ 
uint32_t Utftext::iterate(){
	size_t i = index;
	size_t n = file_content.size();
	if(i >= n){
		return -1;
	}		
	auto val1 = file_content[i];
	if((val1 & 0b1000'0000) == 0) {
		index += 1;
		return val1;
	}else if((val1 & 0b1110'0000) == 0b1100'0000){
		if(i+1 >= n || ((file_content[i+1] & 0b1100'0000) != 0b1000'0000)){
			return -1;
		}
		index += 2;
		auto val2 = file_content[i+1];
		return ((val1 & 0b0001'1111) << 6) | (val2 & 0b0011'1111);

	}else if((val1 & 0b1111'0000) == 0b1110'0000){
		if(i+2 >= n || ((file_content[i+1] & 0b1100'0000) != 0b1000'0000) 
					|| ((file_content[i+2] & 0b1100'0000) != 0b1000'0000)){
			return -1;
		}
		index += 3;
		auto val2 = file_content[i+1];
		auto val3 = file_content[i+2];
		return ((val1 & 0b0000'1111) << 12) | ((val2 & 0b0011'1111) << 6) | (val3 & 0b0011'1111);

	}else if((val1 & 0b1111'1000) == 0b1111'0000){
		if(i+3 >= n || ((file_content[i+1] & 0b1100'0000) != 0b1000'0000) 
					|| ((file_content[i+2] & 0b1100'0000) != 0b1000'0000)
					|| ((file_content[i+3] & 0b1100'0000) != 0b1000'0000)){
			return -1;
		}
		index += 4;
		auto val2 = file_content[i+1];
		auto val3 = file_content[i+2];
		auto val4 = file_content[i+3];
		return ((val1 & 0b0000'0111) << 18) | ((val2 & 0b0011'1111) << 12) | 
			((val3 & 0b0011'1111) << 6) | (val4 & 0b0011'1111);
	}
	return -1;
}


/*
Funtion that takes in one utf character and performs following transformation
-  upper to lower case convertion.
-  removal of accents.
*/

std::vector<uint32_t> Utftext::normalize_char(uint32_t ch){
	auto t = _nfd_transformation(ch);
	for(size_t i = 0; i < t.size(); i++){
		t[i] = _to_lowercase(t[i]);
	}
	return t; 
}

/*
* Funtion that converts 32 bit codepoint to utf-8 data chunks.
*/
std::vector<uint8_t> Utftext::convert2utf_8(uint32_t ch){
	std::vector<uint8_t> utf_8_data;
	if(ch < 0x007f){
		uint8_t val1 = static_cast<uint8_t>(ch);
		utf_8_data.push_back(val1);
	}else if(ch <= 0x07ff){
		uint8_t val1 = static_cast<uint8_t>((ch & 0b111'1100'0000) >> 6) | 0b1100'0000;
		uint8_t val2 = static_cast<uint8_t>(ch & 0b000'0011'1111) | 0b1000'0000;
		utf_8_data.push_back(val1);
		utf_8_data.push_back(val2);
	}else if(ch <= 0xffff){
		uint8_t val1 = static_cast<uint8_t>((ch & 0b1111'0000'0000'0000) >> 12) | 0b1110'0000;
		uint8_t val2 = static_cast<uint8_t>((ch & 0b0000'1111'1100'0000) >> 6) | 0b1000'0000;
		uint8_t val3 = static_cast<uint8_t>(ch & 0b0000'0000'0011'1111) | 0b1000'0000;
		utf_8_data.push_back(val1);
		utf_8_data.push_back(val2);
		utf_8_data.push_back(val3);
	}else if(ch <= 0x10ffff){
		uint8_t val1 = static_cast<uint8_t>((ch & 0b1'1100'0000'0000'0000'0000) >> 18) | 0b1111'0000;
		uint8_t val2 = static_cast<uint8_t>((ch & 0b0'0011'1111'0000'0000'0000) >> 12) | 0b1000'0000;
		uint8_t val3 = static_cast<uint8_t>((ch & 0b0'0000'0000'1111'1100'0000) >> 6) | 0b1000'0000;
		uint8_t val4 = static_cast<uint8_t>(ch & 0b0'0000'0000'0000'0011'1111)| 0b1000'0000;
		utf_8_data.push_back(val1);
		utf_8_data.push_back(val2);
		utf_8_data.push_back(val3);
		utf_8_data.push_back(val4);
	}
	return utf_8_data;	
}

int main(){
	// converting the file
	Utftext text("test_file.txt");
	text.normalize();
	text.write_back("temp.txt");
	return 0;
}