#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum LicenseType
{
	Professional = 1,
	Educational = 3,
	Persional = 4
};

struct License
{
	LicenseType type = Professional;
	char userName[128];
	int iMajorVersion;
	int iMinorVersion;
	int count = 1;
};

static int SplitStringEx(char *src, const char *dest[], size_t maxStrings, const char *delimiter)
{
	if (!strstr(src, delimiter))
		return 0;

	int i = 0;
	char *token = strsep(&src, delimiter);
	while (token && i < maxStrings)
	{
		dest[i] = token;
		token = strsep(&src, delimiter);
		i++;
	}

	return i;
}

static void EncryptBytes(int key, unsigned char *input, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		input[i] = input[i] ^ ((key >> 8) & 0xFF);
		key = (input[i] & key) | 0x482D;
	}
}

static char *VariantBase64Encode(const unsigned char *src, size_t len)
{
	const unsigned char Base64Table[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

	int numBlocks = len / 3;
	int leftBytes = len % 3;

	int outLen = (numBlocks * 4) + (leftBytes ? 4 : 0);
	char *result = (char*)malloc(outLen + 1);
	char block[5];

	for (int i = 0; i < numBlocks; i++)
	{
		int coding_int = (src[3*i+2] << 16) | (src[3*i+1] << 8) | (src[3*i]);
		block[0] = Base64Table[coding_int & 0x3F];
		block[1] = Base64Table[(coding_int >> 6) & 0x3F];
		block[2] = Base64Table[(coding_int >> 12) & 0x3F];
		block[3] = Base64Table[(coding_int >> 18) & 0x3F];
		block[4] = '\0';
		strcat(result, block);
	}

	if (leftBytes == 1)
	{
		int coding_int = src[3 * numBlocks];
		block[0] = Base64Table[coding_int & 0x3F];
		block[1] = Base64Table[(coding_int >> 6) & 0x3F];
		block[2] = '\0';
		strcat(result, block);
	}
	else if (leftBytes == 2)
	{
		int coding_int = (src[3*numBlocks+1] << 8) | (src[3*numBlocks] << 0);
		block[0] = Base64Table[coding_int & 0x3F];
		block[1] = Base64Table[(coding_int >> 6) & 0x3F];
		block[2] = Base64Table[(coding_int >> 12) & 0x3F];
		block[3] = '\0';
		strcat(result, block);
	}

	return result;
}

static char *GenerateLicense(const License &license)
{
	char buffer[512];
	snprintf((char*)buffer, sizeof(buffer), "%d#%s|%d%d#%d#%d3%d6%d#%d#%d#%d#", 
		license.type, license.userName, 
		license.iMajorVersion, license.iMinorVersion, 
		license.count, 
		license.iMajorVersion, license.iMinorVersion, license.iMinorVersion, 
		0,	// Unknown
		0,	// No Games flag. 0 means "NoGames = false". But it does not work.
		0);	// No Plugins flag. 0 means "NoPlugins = false". But it does not work.

	EncryptBytes(0x787, (unsigned char*)buffer, strlen(buffer));
	return VariantBase64Encode((unsigned char*)buffer, strlen(buffer));
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("use: %s <userName> <Version>\n", argv[0]);
		return 0;
	}

	License license;
	snprintf(license.userName, sizeof(license.userName), "%s", argv[1]);

	char *ver = strdup(argv[2]);
	const char *buffer[2];
	int numSplit = SplitStringEx(ver, buffer, 2, ".");
	license.iMajorVersion = atoi(buffer[0]);
	license.iMinorVersion = atoi(buffer[1]);

	char *result = GenerateLicense(license);
	printf("%s\n", result);

	FILE *pFile = fopen("Pro.key", "w");
	fputs(result, pFile);

	fclose(pFile);
	free(ver);
	free(result);

	system("zip -m Custom.mxtpro Pro.key");
	return 0;
}
