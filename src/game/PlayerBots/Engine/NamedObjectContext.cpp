#include "NamedObjectContext.h"
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

void Qualified::Qualify(int qual)
{
    std::ostringstream out;
    out << qual;
    qualifier = out.str();
}

std::string const Qualified::MultiQualify(const std::vector<std::string>& qualifiers, const std::string& separator, const std::string& brackets)
{
    std::stringstream out;
    for (size_t i = 0; i < qualifiers.size(); ++i)
    {
        const std::string& qualifier = qualifiers[i];
        if (i == qualifiers.size() - 1)
        {
            out << qualifier;
        }
        else
        {
            out << qualifier << separator;
        }
    }

    if (brackets.empty())
    {
        return out.str();
    }
    else
    {
        return brackets[0] + out.str() + brackets[1];
    }
}

std::vector<std::string> Qualified::getMultiQualifiers(const std::string& qualifier1)
{
    std::istringstream iss(qualifier1);
    return {std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
}

int32 Qualified::getMultiQualifier(const std::string& qualifier1, uint32 pos)
{
    auto qualifiers = getMultiQualifiers(qualifier1);
    if (pos >= qualifiers.size())
        return 0;
    return std::stoi(qualifiers[pos]);
}
