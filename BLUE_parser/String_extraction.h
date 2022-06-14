#pragma once
class String_extraction
{
public:
	String_extraction();

	void add(char* str, size_t len);
	void store_to_file(const char* file_name);

	~String_extraction();
};

