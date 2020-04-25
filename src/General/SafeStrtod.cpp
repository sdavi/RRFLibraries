/*
 * strtod.cpp
 *
 *  Created on: 4 Apr 2018
 *      Author: David
 *
 * This is a replacement for strtod() and strtof() in the C standard library. Those versions have two problems:
 * 1. They are not reentrant. We can make them so by defining configUSE_NEWLIB_REENTRANT in FreeRTOS, but that makes the tasks much bigger.
 * 2. They allocates and releases heap memory, which is not nice.
 *
 * Limitations of these versions
 * 1. Rounding to nearest double may not always be correct.
 * 2. Does not handle overflow for stupidly large numbers correctly.
 */

#include <cctype>
#include <cmath>
#include <climits>
#include <cstdlib>

#include "SafeStrtod.h"
#undef strtoul		// Undo the macro definition of strtoul in SafeStrtod.h so that we can call it in this file

#include "../Math/Power.h"

double SafeStrtod(const char *s, const char **p) noexcept
{
	// 0. Skip white space
	while (*s == ' ' || *s == '\t')
	{
		++s;
	}

	// 1. Check for a sign
	const bool negative = (*s == '-');
	if (negative || *s == '+')
	{
		++s;
	}

	// 2. Read digits before decimal point, E or e. We use floating point for this in case of very large numbers.
	double valueBeforePoint = 0.0;
	while (isdigit(*s))
	{
		valueBeforePoint = (valueBeforePoint * 10) + (*s - '0');
		++s;
	}

	// 3. Check for decimal point before E or e
	unsigned long valueAfterPoint = 0;
	long digitsAfterPoint = 0;
	if (*s == '.')
	{
		++s;

		// 3b. Read the digits (if any) after the decimal point
		bool overflowed = false;
		while (isdigit(*s))
		{
			if (!overflowed)
			{
				const unsigned int digit = *s - '0';
				if (valueAfterPoint <= (ULONG_MAX - digit)/10)
				{
					valueAfterPoint = (valueAfterPoint * 10) + digit;
					++digitsAfterPoint;
				}
				else
				{
					overflowed = true;
					if (digit >= 5 && valueAfterPoint != ULONG_MAX)
					{
						++valueAfterPoint;			// do approximate rounding
					}
				}
			}
			++s;
		}
	}

	// 5. Check for exponent part
	long exponent = 0;
	if (toupper(*s) == 'E')
	{
		++s;

		// 5a. Check for signed exponent
		const bool expNegative = (*s == '-');
		if (expNegative || *s == '+')
		{
			++s;
		}

		// 5b. Read exponent digits
		while (isdigit(*s))
		{
			exponent = (10 * exponent) + (*s - '0');	// could overflow, but anyone using such large numbers is being very silly
			++s;
		}

		if (expNegative)
		{
			exponent = -exponent;
		}
	}

	// 6. Compute the composite value
	double retvalue;
	if (valueAfterPoint != 0)
	{
		if (valueBeforePoint == (double)0.0L)
		{
			retvalue = TimesPowerOf10((double)valueAfterPoint, exponent - digitsAfterPoint);
		}
		else
		{
			retvalue = TimesPowerOf10(TimesPowerOf10((double)valueAfterPoint, -digitsAfterPoint) + valueBeforePoint, exponent);
		}
	}
	else
	{
		retvalue = TimesPowerOf10(valueBeforePoint, exponent);
	}

	// 7. Set end pointer
	if (p != nullptr)
	{
		*p = s;
	}

	// 8. Adjust sign if necessary
	return (negative) ? -retvalue : retvalue;
}

float SafeStrtof(const char *s, const char **p) noexcept
{
	return (float)SafeStrtod(s, p);
}

unsigned long SafeStrtoul(const char *s, const char **endptr, int base) noexcept
{
	// strtoul() accepts a leading minus-sign, which we don't want to allow
	while (*s == ' ' || *s == '\t')
	{
		++s;
	}
	if (*s == '-')
	{
		if (endptr != nullptr)
		{
			*endptr = s;
		}
		return 0;
	}
	return strtoul(s, const_cast<char**>(endptr), base);
}

unsigned long SafeStrtoul(char *s, char **endptr, int base) noexcept
{
	// strtoul() accepts a leading minus-sign, which we don't want to allow
	while (*s == ' ' || *s == '\t')
	{
		++s;
	}
	if (*s == '-')
	{
		if (endptr != nullptr)
		{
			*endptr = s;
		}
		return 0;
	}
	return strtoul(s, endptr, base);
}

// End
