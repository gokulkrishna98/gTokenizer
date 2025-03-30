#include "tokenizer.h"
#include <fstream>
#include <vector>


struct Utftext {
	std::unordered_map<unsigned char, std::vector<unsigned char>> decomposition_map;
	std::vector<unsigned char> file_content;

	void _parse_decomposition_map(const char* filename);
	Utftext(const char *filename); 
	unsigned int get(size_t i);
	unsigned int normalize_char(unsigned int ch);
};

/*
It is a helper function that populates the decomposition map
- Parse the UnicodeData.txt (from : https://www.unicode.org/Public/14.0.0/ucd/UnicodeData.txt)
  line by line.
- Ignore the decomposition which has <compat> tag, as they belong to 
  compatiblity normalization (nfkc/nfkd).
- populate the decomposition map which maps to nfd normalization.
*/
void Utftext::_parse_decomposition_map(const char* filename){
	// TODO:
	return;	
}

Utftext::Utftext(const char * filename){
	std::ifstream file(filename, std::ios::binary);
	if(!file.is_open()){
		fprintf(stderr, "Error opening file\n");
		exit(1);
	}
	char byte;
	while (file.get(byte)) {
	    file_content.push_back(static_cast<unsigned char>(byte));
	}	
	file.close();
}

/*
If the i is valid start index, then it parses the utf codepoint value,
incase it is not valid then we return -1 i.e 0xffff'ffff (which is not used);
*/ 
unsigned int Utftext::get(size_t i){
	size_t n = file_content.size();
	if(i >= n){
		return -1;
	}		
	auto val1 = file_content[i];
	if((val1 & 0b1000'0000) == 0) {
		return val1;
	}else if((val1 & 0b1110'0000) == 0b1100'0000){
		if(i+1 >= n || ((file_content[i+1] & 0b1100'0000) != 0b1000'0000)){
			return -1;
		}
		auto val2 = file_content[i+1];
		return ((val1 & 0b0001'1111) << 6) | (val2 & 0b0011'1111);

	}else if((val1 & 0b1111'0000) == 0b1110'0000){
		if(i+2 >= n || ((file_content[i+1] & 0b1100'0000) != 0b1000'0000) 
					|| ((file_content[i+2] & 0b1100'0000) != 0b1000'0000)){
			return -1;
		}
		auto val2 = file_content[i+1];
		auto val3 = file_content[i+2];
		return ((val1 & 0b0000'1111) << 12) | ((val2 & 0b0011'1111) << 6) | (val3 & 0b0011'1111);

	}else if((val1 & 0b1111'1000) == 0b1111'0000){
		if(i+3 >= n || ((file_content[i+1] & 0b1100'0000) != 0b1000'0000) 
					|| ((file_content[i+2] & 0b1100'0000) != 0b1000'0000)
					|| ((file_content[i+3] & 0b1100'0000) != 0b1000'0000)){
			return -1;
		}

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
unsigned int Utftext::normalize_char(unsigned int ch){

}

int main(){
	// converting the file
	Utftext text("test_file.txt");
	auto t = text.get(28);
	printf("size - %ld\n", text.file_content.size());
	printf("text[1] - %x\n", t);
	return 0;
}