#ifndef DATATYPES_H
#define DATATYPES_H

#include <cstdint>
#include <utility>
#include <vector>

typedef std::vector<uint8_t> DataBuffer;
typedef std::vector<uint8_t>::iterator DataIterator;
typedef std::pair<DataIterator,DataIterator> DataDescriptor;
typedef int16_t SzInteger;

#endif /*DATATYPES_H*/
