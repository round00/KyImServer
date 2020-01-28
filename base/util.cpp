#include "util.h"
#include <unistd.h>

std::string trim(const std::string& s) 
{
	if (s.empty())
		return s;

	int b = 0, e = s.length()-1;
	//trim before
	while (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')b++;
	while (s[e] == ' ' || s[e] == '\t' || s[e] == '\r' || s[e] == '\n')e--;
	
	return s.substr(b, e-b+1);
}
void crash()
{
    int* p = nullptr;
    *p = 0;
}

bool daemon_run()
{
    return ::daemon(1, 0) == 0;
}
