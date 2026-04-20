#pragma once

#include <string.h>
#include <vector>
using namespace std;

class DpnpCommonEventParam
{
public:
	virtual int Serialize(void* buf, int offset, int bufLen) {
		return 0;
	}

	virtual int Deserialize(void* buf, int offset, int bufLen)
	{
		return 0;
	}

	template<typename T>
	static int Serialize(const T& value, void* buf, int offset, int length)
	{
		size_t size = sizeof(value);
		if (size > (size_t)offset + (size_t)length)
		{
			return 0;
		}
		memcpy((char*)buf + offset, &value, size);
		return (int)size;
	}

	template<typename T>
	static int Deserialize(T& value, void* buf, int offset, int length)
	{
		size_t size = sizeof(value);
		if (size > (size_t)offset + (size_t)length)
		{
			return 0;
		}
		memcpy(&value, (char*)buf + offset, size);
		return (int)size;
	}

	static int Deserialize(std::string& value, void* buf, int offset, int length)
	{
		size_t size = sizeof(int);
		if (size > (size_t)offset + (size_t)length)
		{
			return 0;
		}
		int strLen = 0;
		auto result = Deserialize<int>(strLen, buf, offset, length);
		size_t realLen = (size_t)strLen + 1;
		size += realLen;
		if (size > (size_t)offset + (size_t)length)
		{
			return 0;
		}
		auto strBuf = new char[realLen];
		memset(strBuf, 0, realLen);

		memcpy(strBuf, (char*)buf + offset + sizeof(strLen), realLen);

		result += strLen;

		value = string(strBuf, strLen);

		delete[] strBuf;

		return result;
	}

	template<typename T>
	static int Deserialize(std::vector<T>& value, void* buf, int offset, int length)
	{
		size_t size = sizeof(int);
		if (size > (size_t)offset + (size_t)length)
		{
			return 0;
		}
		int count = 0;
		auto read_len = Deserialize<int>(count, buf, offset, length);
		size += count * sizeof(T);
		if (size > (size_t)offset + (size_t)length)
		{
			return 0;
		}

		for (int i = 0; i < count; ++i)
		{
			T item;
			read_len += Deserialize(item, buf, offset + read_len, length);

			value.push_back(item);
		}


		return read_len;
	}

	static int Serialize(const string& value, void* buf, int offset, int length)
	{
		size_t size = sizeof(int) + value.length();
		if (size > (size_t)offset + (size_t)length)
		{
			return 0;
		}
		int len = (int)value.length();
		int result = Serialize<int>(len, buf, offset, length);
		
		memcpy((char*)buf + offset + sizeof(len), value.c_str(), len);

		result += len;
		
		return result;
	}

	template<typename T>
	static int Serialize(const std::vector<T>& value, void* buf, int offset, int length)
	{
		int count = (int)value.size();
		size_t size = sizeof(int) + count * sizeof(T);
		if (size > (size_t)offset + (size_t)length)
		{
			return 0;
		}
		int write_len = Serialize<int>(count, buf, offset, length);

		for (int i = 0; i < count; ++i)
		{
			write_len += Serialize(value[i], buf, offset + write_len, length);
		}

		return write_len;
	}

};

template<typename T0>
class DpnpCommonEventParam_T1 : public DpnpCommonEventParam
{
public:
	T0 _value0;

	DpnpCommonEventParam_T1(const DpnpCommonEventParam_T1& other)
	{
		this->_value0 = other._value0;
	}

	DpnpCommonEventParam_T1(const T0& value0={})
		: _value0(value0)
	{

	}

	virtual int Serialize(void* buf, int offset, int bufLen)
	{
		int len = 0;

		len += DpnpCommonEventParam::Serialize(_value0, buf, offset + len, bufLen);

		return len;
	}

	virtual int Deserialize(void* buf, int offset, int bufLen)
	{
		int len = 0;
		len += DpnpCommonEventParam::Deserialize(_value0, buf, offset + len, bufLen);

		return len;
	}

};

template<typename T0, typename T1>
class DpnpCommonEventParam_T2 : public DpnpCommonEventParam
{
public:
	T0 _value0;
	T1 _value1;

	DpnpCommonEventParam_T2(const DpnpCommonEventParam_T2& other)
	{
		this->_value0 = other._value0;
		this->_value1 = other._value1;
	}

	DpnpCommonEventParam_T2(const T0& value0={}, const T1& value1={})
		: _value0(value0), _value1(value1)
	{

	}

	virtual int Serialize(void* buf, int offset, int bufLen)
	{
		int len = 0, temp = 0;

		len += temp = DpnpCommonEventParam::Serialize(_value0, buf, offset + len, bufLen);
		if (!temp) return 0;
		len += temp = DpnpCommonEventParam::Serialize(_value1, buf, offset + len, bufLen);
		if (!temp) return 0;

		return len;
	}

	virtual int Deserialize(void* buf, int offset, int bufLen)
	{
		int len = 0, temp = 0;

		len += temp = DpnpCommonEventParam::Deserialize(_value0, buf, offset + len, bufLen);
		if (!temp) return 0;
		len += temp = DpnpCommonEventParam::Deserialize(_value1, buf, offset + len, bufLen);
		if (!temp) return 0;

		return len;
	}
};

template<typename T0, typename T1, typename T2>
class DpnpCommonEventParam_T3 : public DpnpCommonEventParam
{

public:
	T0 _value0;
	T1 _value1;
	T2 _value2;

	DpnpCommonEventParam_T3(const DpnpCommonEventParam_T3& other)
	{
		this->_value0 = other._value0;
		this->_value1 = other._value1;
		this->_value2 = other._value2;
	}

	DpnpCommonEventParam_T3(const T0& value0 = {}, const T1& value1 = {}, const T2& value2 = {})
		: _value0(value0), _value1(value1), _value2(value2)
	{

	}

	virtual int Serialize(void* buf, int offset, int bufLen)
	{
		int len = 0, temp = 0;

		len += temp = DpnpCommonEventParam::Serialize(_value0, buf, offset + len, bufLen);
		if (!temp) return 0;
		len += temp = DpnpCommonEventParam::Serialize(_value1, buf, offset + len, bufLen);
		if (!temp) return 0;
		len += temp = DpnpCommonEventParam::Serialize(_value2, buf, offset + len, bufLen);
		if (!temp) return 0;

		return len;
	}

	virtual int Deserialize(void* buf, int offset, int bufLen)
	{
		int len = 0, temp = 0;

		len += temp = DpnpCommonEventParam::Deserialize(_value0, buf, offset + len, bufLen);
		if (!temp) return 0;
		len += temp = DpnpCommonEventParam::Deserialize(_value1, buf, offset + len, bufLen);
		if (!temp) return 0;
		len += temp = DpnpCommonEventParam::Deserialize(_value2, buf, offset + len, bufLen);
		if (!temp) return 0;

		return len;
	}
};


template<typename T0, typename T1, typename T2, typename T3>
class DpnpCommonEventParam_T4 : public DpnpCommonEventParam
{
public:
	T0 _value0;
	T1 _value1;
	T2 _value2;
	T3 _value3;

	DpnpCommonEventParam_T4(const DpnpCommonEventParam_T4& other)
	{
		this->_value0 = other._value0;
		this->_value1 = other._value1;
		this->_value2 = other._value2;
		this->_value3 = other._value3;
	}

	DpnpCommonEventParam_T4(const T0& value0 = {}, const T1& value1 = {}, const T2& value2 = {}, const T3& value3 = {})
		: _value0(value0), _value1(value1), _value2(value2), _value3(value3)
    {

    }

    virtual int Serialize(void* buf, int offset, int bufLen)
    {
		int len = 0, temp = 0;

		len += temp = DpnpCommonEventParam::Serialize(_value0, buf, offset + len, bufLen);
		if (!temp) return 0;
		len += temp = DpnpCommonEventParam::Serialize(_value1, buf, offset + len, bufLen);
		if (!temp) return 0;
		len += temp = DpnpCommonEventParam::Serialize(_value2, buf, offset + len, bufLen);
		if (!temp) return 0;
		len += temp = DpnpCommonEventParam::Serialize(_value3, buf, offset + len, bufLen);
		if (!temp) return 0;

		return len;
    }

    virtual int Deserialize(void* buf, int offset, int bufLen)
    {
		int len = 0, temp = 0;

		len += temp = DpnpCommonEventParam::Deserialize(_value0, buf, offset + len, bufLen);
		if (!temp) return 0;
		len += temp = DpnpCommonEventParam::Deserialize(_value1, buf, offset + len, bufLen);
		if (!temp) return 0;
		len += temp = DpnpCommonEventParam::Deserialize(_value2, buf, offset + len, bufLen);
		if (!temp) return 0;
		len += temp = DpnpCommonEventParam::Deserialize(_value3, buf, offset + len, bufLen);
		if (!temp) return 0;

        return len;
    }
};