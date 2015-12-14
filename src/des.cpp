/**
 * @file 3des.cpp
 * @brief Implementation of Data Encryption Standard (DES) defined in FIPS 46-3
 */

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <sstream>
#include <iomanip>

#include "foxbox.h"
#include "des.h"

using namespace std;
namespace Foxbox {namespace DES
{
/**
 * Permute a sequence of M bits into a sequence of N bits
 * @param input - Original bits
 * @param P - array of N indices into the M bits
 * @returns Sequence of N bits determined by P
 */
template <size_t N,size_t M>
bitset<N> Permute(const bitset<M> & input, int8_t P[N])
{
	bitset<N> output = 0;
	for (size_t i = 0; i < N; ++i)
	{
		output[i] = input[P[i]-1];
	}
	return output;
}

/**
 * Permute the input prior to the cipher operations
 * @param input - The input bits
 * @returns Permuted input
 */
bitset<64> PrePermute(const bitset<64> & input)
{

	static int8_t perm[] = {58,50,42,34,26,18,10,2,
					  60,52,44,36,28,20,12,4,
					  62,54,46,38,30,22,14,6,
					  64,56,48,40,32,24,16,8,
					  57,49,41,33,25,17,9,1,
					  59,51,43,35,27,19,11,3,
					  61,53,45,37,29,21,13,5,
					  63,55,47,39,31,23,15,7};
	bitset<64> output = Permute<64,64>(input, perm);
	return output;
}

/**
 * Permute the output of the cipher operations. Inverse of PrePermute.
 * @param input - output of cipher operations to be permuted
 * @returns Permutation of input
 */
bitset<64> PostPermute(const bitset<64> & input)
{
	static int8_t perm[] = {40,8,48,16,56,24,64,32,
					  39,7,47,15,55,23,63,31,
					  38,6,46,14,54,22,62,30,
					  37,5,45,13,53,21,61,29,
					  36,4,44,12,52,20,60,28,
					  35,3,43,11,51,19,59,27,
					  34,2,42,10,50,18,58,26,
					  33,1,41,9,49,17,57,25};
	bitset<64> output = Permute<64,64>(input, perm);
	return output;	
}

/**
 * Perform one round of the DES cipher
 * @param R - The R parameter
 * @param K - The current key selection of 48 bits
 * @returns - Ciphered R with K
 */
bitset<32> Cipher(const bitset<32> & R, const bitset<48> & K)
{

	static int8_t Eperm[] = {32,1,2,3,4,5,
					  4,5,6,7,8,9,
					  8,9,10,11,12,13,
					  12,13,14,15,16,17,
					  16,17,18,19,20,21,
					  20,21,22,23,24,25,
					  24,25,26,27,28,29,
					  28,29,30,31,32,1};


	static int8_t S1[] = {14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7,
				0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8,
				4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0,
				15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13};
	static int8_t S2[] = {15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10,
				3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5,
				0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15,
				13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9};
	static int8_t S3[] = {10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8,
				13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1,
				13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7,
				1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12};
	static int8_t S4[] = {7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15,
				13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9,
				10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4,
				3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14};
	static int8_t S5[] = {2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9,
				14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6,
				4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14,
				11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3};
	static int8_t S6[] = {12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11,
				10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8,
				9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6,
				4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13};
	static int8_t S7[] = {4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1,
				13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6,
				1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2,
				6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12};
	static int8_t S8[] = {13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7,
				1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2,
				7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8,
				2,1,14,4,10,8,13,15,12,9,0,3,5,6,11};
	int8_t * S[] = {S1,S2,S3,S4,S5,S6,S7,S8};
	
	static int8_t Pperm[] = {16,7,20,21,
				   29,12,28,17,
				   1,15,23,26,
				   5,18,31,10,
				   2,8,24,14,
				   32,27,3,9,
				   19,13,30,6,
				   22,11,4,25};
		
	bitset<48> E = Permute<48,32>(R, Eperm);
	
	bitset<48> B = E ^ K;
	bitset<32> Sfinal(0);
	for (int i = 0; i < 8; ++i)
	{
		bitset<6> Bi;
		for (int j=0; j < 6; j++)
			Bi[j] = B[i*6+j];
			
		int8_t row = Bi[0];
		row |= (Bi[5] << 1);
		int8_t column = 0;
		for (int j = 0; j < 4; ++j)
		{
			column |= (Bi[j+1] << j);
		}
		bitset<4> Si(S[i][15*row+column]);
		for (int j=0; j<4; ++j)
		{
			Sfinal[i*4 + j] = Si[j];
		}
	}
	//Debug("S: %s", Int64ToBinary(Sfinal));
	bitset<32> P = Permute<32,32>(Sfinal, Pperm);
	return P;
}

/**
 * Construct the schedule of 48 bit selections from the key
 * @param key - The original key
 * @param K - Array to be filled with 16 different 48 bit selections from key
 */
void KeySchedule(const bitset<64> & key, bitset<48> K[])
{
	int8_t PC1[] = {57,49,41,33,25,17,9,
					1,58,50,42,34,26,18,
					10,2,59,51,43,35,27,
					19,11,3,60,52,44,36,
					63,55,47,39,31,23,15,
					7,62,54,46,38,30,22,
					14,6,61,53,45,37,29,
					21,13,5,28,20,12,4};
					
	int8_t PC2[] = {14,17,11,24,1,5,
					3,28,15,6,21,10,
					23,19,12,4,26,8,
					16,7,27,20,13,2,
					41,52,31,37,47,55,
					30,40,51,45,33,48,
					44,49,39,56,34,53,
					46,42,50,36,29,32};
					
	bitset<56> CD = Permute<56,64>(key, PC1);
	//Debug("CD[0]: %s", Int64ToBinary(CD,7));
	
	int8_t ls[] = {1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1};
	for (int i = 1; i < 16; ++i)
	{
		for (int j=0; j < ls[i]; ++j)
		{
			int8_t CD28 = CD[27];
			int8_t CD56 = CD[55];
			CD <<= 1;
			CD[1] = CD28;
			CD[28] = CD56;
		}
		//Debug("CD[%d]: %s", i, Int64ToBinary(CD,7));
		K[i] = Permute<48,56>(CD, PC2);
	}
	
}

/**
 * Encrypt one 64 bit block with DES
 * @param input - The block to encrypt
 * @param key - The key to use
 * @param decrypt - Indicate whether to decrypt
 * @returns Encrypted block of 64 bits
 */
bitset<64> EncryptBlock(const bitset<64> & input, const bitset<64> & key, bool decrypt)
{

	bitset<48> K[16];
	for (int i = 0; i < 16; ++i) K[i] = 0;
	KeySchedule(key, K);
	bitset<64> output = PrePermute(input);
	bitset<32> L;
	bitset<32> R;
	for (int i = 0; i < 32; ++i)
	{
		L[i] = output[i];
		R[i] = output[32+i];
	}
	
	
	if (decrypt)
	{
		for (int i = 15; i >= 0; --i)
		{
			bitset<32> Rp = R;
			R = L ^ Cipher(R,K[i]);
			L = Rp;			
		}
	}
	else
	{
		for (int i = 0; i < 16; ++i)
		{
			bitset<32> Rp = R;
			R = L ^ Cipher(R,K[i]);
			L = Rp;
		}
	}
	
	for (int i = 0; i < 32; ++i)
	{
		output[i] = R[i];
		output[i+32] = L[i];
	}
	output = PostPermute(output);
	return output;
}

/**
 * Encrypts a string of characters using DES
 * @param input - The input characters
 * @param output - Output encrypted string will be appended to this string
 */
void EncryptString(const string & input, string & output, const bitset<64> & key, bool decrypt)
{
	for (size_t i = 0; i < input.size(); i+=8)
	{
		int64_t block = input[i] & 0xFF;
		for (size_t j = 1; j < 8; ++j)
		{
			//Debug("Input %lx", block);
			if (i+j > input.size()) break;
			int64_t c = input[i+j] & 0xFF;
			c <<= 8*j;
			block |= c;
			//Debug("Input Block %d: %lx -> %lx", j, c, block);
		}
		//Debug("Input Block %d-%d: %lx",i, i+7,block);
		
		int64_t block_out = EncryptBlock(block, key, decrypt).to_ulong();
		//Debug("Output Block %d-%d: %lx",i, i+7, block_out);
		//assert(EncryptBlock(block_out, key, !decrypt) == block);
		for (size_t j=0; j < 8; ++j)
		{
			int64_t c = block_out & 0xFF;
			//Debug("Output Block %d: %lx, %c", j, c, (int8_t)c);
			output += (char)c;
			block_out >>= 8;
		}
		
	}
}

void StringToHex(const string & input, string & output)
{
	stringstream s;
	for (size_t i = 0; i < input.size(); ++i)
	{
		s << std::hex << (int)(input[i]) << ",";
	}
	output += s.str();
}


Socket::Socket(Foxbox::Socket & socket, const bitset<64> & keyA, const bitset<64> & keyB, const bitset<64> & keyC) : Foxbox::Socket(socket),
	m_keyA(keyA), m_keyB(keyB), m_keyC(keyC)
{
	
}

Socket::~Socket()
{
	
}
int Socket::SendRaw(const void * buffer, size_t bytes)
{
	char * b = (char*)buffer;
	string input;
	for (size_t i = 0; i < bytes; ++i)
	{
		input += (*b);
		b++;
	}
	string output1;
	string output2;
	string output3;
	DES::EncryptString(input, output1, m_keyA, false);
	DES::EncryptString(output1, output2, m_keyB, true);
	DES::EncryptString(output2, output3, m_keyC, false);
	string h;
	StringToHex(output3, h);
	//Debug("Sending %s", h.c_str());
	return Foxbox::Socket::SendRaw(output3.c_str(), output3.size());
}

bool Socket::Send(const char * message, ...)
{
	va_list ap;
	va_start(ap, message);
	int size = vsnprintf(NULL, 0, message, ap);
	va_end(ap);
	//Debug("Message is \"%s\"", message);
	char * input = new char[size+1];
	
	va_start(ap, message);
	vsnprintf(input, size+1, message, ap);
	va_end(ap);
	
	string output1;
	string output2;
	string output3;
	DES::EncryptString(string(input), output1, m_keyA, false);
	delete [] input;
	
	DES::EncryptString(output1, output2, m_keyB, true);
	DES::EncryptString(output2, output3, m_keyC, false);
	string h;
	StringToHex(output3, h);
	//Debug("Sending %s", h.c_str());
	//Debug("Original string: \"%s\"", orig.c_str());
	return Foxbox::Socket::Send(output3.c_str());
}

bool Socket::GetToken(string & buffer, const char * delims, double timeout, bool inclusive)
{
	do
	{
		if (!Get(buffer, 1, timeout)) return false;
		//Debug("Got \"%s\"", buffer.c_str());
		if (strchr(delims, buffer[buffer.size()-1]))
		{
			if (!inclusive) buffer[buffer.size()-1] = '\0';
			return true;
		}
	}
	while (true);
	return true;
}

bool Socket::Get(string & buffer, size_t num_chars, double timeout)
{
	size_t i = 0;
	for (; i < num_chars && m_buffer_index < 8; ++i)
		buffer += m_buffer[m_buffer_index++];
	
	if (i >= num_chars)
		return true;
	
	
	size_t to_get = 8;
	string input;
	string output1;
	string output2;
	string output3;
	for (size_t j = 0; j < to_get; ++j)
	{
		if (!Foxbox::Socket::Get(input, 1, timeout))
			return false;

	}
	string h;
	StringToHex(input, h);
	//Debug("Received %s", h.c_str());
	DES::EncryptString(input, output1, m_keyC, true);
	DES::EncryptString(output1, output2, m_keyB, false);
	DES::EncryptString(output2, output3, m_keyA, true);

	for (size_t j = 0; j < 8; ++j)
		m_buffer[j] = output3[j];
	m_buffer_index = 0;
	return Get(buffer, num_chars-i, timeout);
}

}}
/*
int main(int argc, char ** argv)
{
	bitset<64> input = 0x0123456789abcdef;
	bitset<64> key =   0xbaadf00ddeadcafe;
	Debug("Input: %lx", input.to_ulong());
	Debug("key: %lx", key);
	bitset<64> output = Encrypt(input, key, false);
	Debug("Output: %lx", output.to_ulong());
	input = Encrypt(output, key, true);
	Debug("Input: %lx", input.to_ulong());
	return 0;
}
*/
