// Inspired by Chapter 17: Binary Data
// From the book: Software Design by Example
// https://third-bit.com/sdxpy/binary/

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <iostream>
#include <vector>

using namespace std;

void binary_notation()
{
    assert(0b101101 == 45);
    assert(0xF7 == 0b11110111);
    assert(0xF7 == 247);
}

void bitwise_operations()
{
    int mask_low_bits = 0b1111;
    assert((12 & 6) == 4);
    assert((12 | 6) == 14);
    assert((12 ^ 6) == 10);
    assert((~6 & mask_low_bits) == 9);
    assert((12 << 2) == 48);
    assert((12 >> 2) == 3);
}

void unicode_character_info(uint32_t c)
{
    cout << "Value: " << c << endl;
    cout << "Range: ";
    if (c < 128)
    {
        cout << "Valid ASCII: " << (char)c << endl;
    }
    else if (c < 256)
    {
        cout << "Valid ANSI: " << (char)c << endl;
    }
    else if (c < 65536)
    {
        cout << "Valid UTF-16: " << (wchar_t)c << endl;
    }
    else
    {
        cout << "Possible UTF-32" << endl;
    }
    if (c < 256)
    {
        cout << "UTF-8: ";
        if (c < 128)
        {
            cout << "Single byte: " << (char)c << endl;
        }
        else
        {
            cout << "Multi-byte - ";
            if (c & 0b11110000)
            {
                cout << "First of a 4 byte character with 3 bits payload: " << (c & 0b00000111) << endl;
            }
            else if (c & 0b11100000)
            {
                cout << "First of a 3 byte character with 4 bits payload: " << (c & 0b00001111) << endl;
            }
            else if (c & 0b11000000)
            {
                cout << "First of a 2 byte character with 5 bits payload: " << (c & 0b00011111) << endl;
            }
            else if (c & 0b10000000)
            {
                cout << "Continuation byte with 6 bits payload: " << (c & 0b00111111) << endl;
            }
        }
    }
    cout << "--" << endl;
}

void encode_utf8(uint64_t codepoint)
{
    printf("Unicode Codepoint: %lld\n", codepoint);
    if (codepoint < 0x80)
    {
        uint32_t byte1 = (uint32_t)codepoint;
        printf("Single byte: %u (%02x)\n", byte1, byte1);
    }
    else if (codepoint < 0x800)
    {
        uint32_t byte1 = ((codepoint >> 6) & 0b00011111) | 0b11000000;
        uint32_t byte2 = (codepoint & 0b00111111) | 0b10000000;
        printf("Two bytes: %u %u (%02x %02x)\n", byte1, byte2, byte1, byte2);
    }
    else if (codepoint < 0x10000ull)
    {
        uint32_t byte1 = ((codepoint >> 12) & 0b00001111) | 0b11100000;
        uint32_t byte2 = ((codepoint >> 6) & 0b00111111) | 0b10000000;
        uint32_t byte3 = (codepoint & 0b00111111) | 0b10000000;
        printf("Three bytes: %u %u %u (%02x %02x %02x)\n", byte1, byte2, byte3, byte1, byte2, byte3);
    }
    else
    {
        uint32_t byte1 = ((codepoint >> 18) & 0b00000111) | 0b11110000;
        uint32_t byte2 = ((codepoint >> 12) & 0b00111111) | 0b10000000;
        uint32_t byte3 = ((codepoint >> 6) & 0b00111111) | 0b10000000;
        uint32_t byte4 = (codepoint & 0b00111111) | 0b10000000;
        printf("Four bytes: %u %u %u %u (%02x %02x %02x %02x)\n", byte1, byte2, byte3, byte4, byte1, byte2, byte3, byte4);
    }
    cout << "--" << endl;
}

vector<uint8_t> binary_pack(char const* const format, ...)
{
    vector<uint8_t> result;
    va_list args;
    va_start(args, format);
    for (int i = 0; format[i]; i++)
    {
        int count = 0;
        while (isdigit(format[i]))
        {
            count = count * 10 + format[i] - '0';
            i++;
        }
        if (count == 0)
        {
            count = 1;
        }

        if (format[i] == 's')
        {
            char *value = va_arg(args, char *);
            for (int j = 0; j < count; j++)
            {
                result.push_back((uint8_t)(value[j]));
            }
        }
        else
        {
            for (int j = 0; j < count; j++)
            {
                switch (format[i])
                {
                    case 'c':
                    case 'B':
                    {
                        uint8_t value = va_arg(args, uint8_t);
                        result.push_back((uint8_t)(value & 0xFF));
                    } break;

                    case 'h':
                    {
                        uint16_t value = va_arg(args, uint16_t);
                        result.push_back((uint8_t)(value & 0xFF));
                        result.push_back((uint8_t)(value >> 8 & 0xFF));
                    } break;

                    case 'i':
                    {
                        uint32_t value = va_arg(args, uint32_t);
                        result.push_back((uint8_t)(value & 0xFF));
                        result.push_back((uint8_t)(value >> 8 & 0xFF));
                        result.push_back((uint8_t)(value >> 16 & 0xFF));
                        result.push_back((uint8_t)(value >> 24 & 0xFF));
                    } break;

                    case 'd':
                    {
                        assert(sizeof(uint64_t) == sizeof(double));
                        double original = va_arg(args, double);
                        uint64_t value = *((uint64_t*)&original);
                        result.push_back((uint8_t)(value & 0xFF));
                        result.push_back((uint8_t)(value >> 8 & 0xFF));
                        result.push_back((uint8_t)(value >> 16 & 0xFF));
                        result.push_back((uint8_t)(value >> 24 & 0xFF));
                        result.push_back((uint8_t)(value >> 32 & 0xFF));
                        result.push_back((uint8_t)(value >> 40 & 0xFF));
                        result.push_back((uint8_t)(value >> 48 & 0xFF));
                        result.push_back((uint8_t)(value >> 56 & 0xFF));
                    } break;

                    default:
                        throw exception("invalid format");
                }
            }
        }
    }
    va_end(args);
    return result;
}

void binary_unpack(const vector<uint8_t> &bytes, char const* const format, ...)
{
    va_list args;
    va_start(args, format);
    size_t position = 0;
    for (int i = 0; format[i]; i++)
    {
        int count = 0;
        while (isdigit(format[i]))
        {
            count = count * 10 + format[i] - '0';
            i++;
        }
        if (count == 0)
        {
            count = 1;
        }

        if (format[i] == 's')
        {
            char *dest = va_arg(args, char *);
            for (int j = 0; j < count; j++)
            {
                dest[j] = (char)bytes[position++];
            }
            // NOTE: Adding a null terminator is left to caller to avoid undesired memory overwriting!
        }
        else
        {
            for (int j = 0; j < count; j++)
            {
                switch (format[i])
                {
                    case 'c':
                    case 'B':
                    {
                        uint8_t *dest = va_arg(args, uint8_t*);
                        *dest = bytes[position++];
                    } break;

                    case 'h':
                    {
                        uint16_t *dest = va_arg(args, uint16_t*);
                        *dest = bytes[position++];
                        *dest |= bytes[position++] << 8;
                    } break;

                    case 'i':
                    {
                        uint32_t *dest = va_arg(args, uint32_t*);
                        *dest = bytes[position++];
                        *dest |= bytes[position++] << 8;
                        *dest |= bytes[position++] << 16;
                        *dest |= bytes[position++] << 24;
                    } break;

                    case 'd':
                    {
                        assert(sizeof(uint64_t) == sizeof(double));
                        double *dest = va_arg(args, double*);
                        uint64_t temp = 0;
                        temp = bytes[position++];
                        temp |= (uint64_t)(bytes[position++]) << 8;
                        temp |= (uint64_t)(bytes[position++]) << 16;
                        temp |= (uint64_t)(bytes[position++]) << 24;
                        temp |= (uint64_t)(bytes[position++]) << 32;
                        temp |= (uint64_t)(bytes[position++]) << 40;
                        temp |= (uint64_t)(bytes[position++]) << 48;
                        temp |= (uint64_t)(bytes[position++]) << 56;
                        *dest = *((double *)&temp);
                    } break;

                    default:
                        throw exception("invalid format");
                }
            }
        }
    }
    va_end(args);
}

void binary_dump(const vector<uint8_t> &bytes)
{
    printf("Dumping %lld bytes: ", bytes.size());
    for (const auto &byte : bytes)
    {
        printf("%02x ", byte);
    }
    printf("\n");
}

void unicode_examples()
{
    unicode_character_info('A');
    unicode_character_info(0x2605);
    unicode_character_info(0x20AC);
    unicode_character_info(0b11011011);
    unicode_character_info(0b10111101);
    encode_utf8(0x0024);
    encode_utf8(0x00A3);
    encode_utf8(0x0418);
    encode_utf8(0x0939);
    encode_utf8(0x20AC);
    encode_utf8(0xD55C);
    encode_utf8(0x10348ull);
}

void test_pack()
{
    auto bytes = binary_pack("chid", 'A', 131, 100000, 3.14159);
    binary_dump(bytes);
    char c;
    short h;
    int i;
    double d;
    binary_unpack(bytes, "chid", &c, &h, &i, &d);
    assert(c == 'A');
    assert(h == 131);
    assert(i == 100000);
    assert(d == 3.14159);
}

void test_pack_count()
{
    auto bytes = binary_pack("i2d5s", 12345, 1.5, -3.7, "hello");
    binary_dump(bytes);
    int i;
    double d[2];
    char s[5];
    binary_unpack(bytes, "i2d5s", &i, &d[0], &d[1], s);
    assert(i == 12345);
    assert(d[0] == 1.5);
    assert(d[1] == -3.7);
    assert(strncmp("hello", s, 5) == 0);
}

void binary_main()
{
    cout << "Binary Data:" << endl;
    binary_notation();
    bitwise_operations();
    unicode_examples();
    test_pack();
    test_pack_count();
    cout << "All tests passed" << endl;
}