#ifndef _MySecurity_h
#define _MySecurity_h

#define MY_SECURITY_PASSWD		"ABCD"
#define MY_SECURITY_LENGTH		4

class MySecurity
{
	const char passwd[5] = MY_SECURITY_PASSWD;

public:
	static bool getPasswd(char* dst)
	{
		try {
			MySecurity ms;
			::memcpy(dst, ms.passwd, sizeof(char) * MY_SECURITY_LENGTH);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	static bool isPass(const char* src) 
	{
		MySecurity ms;
		for (int i = 0; i < MY_SECURITY_LENGTH; ++i)
			if (src[i] == '\0' || src[i] != ms.passwd[i])
				return false;
		return true;
	}

};

//char MySecurity::passwd[] = MY_SECURITY_PASSWD;

#endif