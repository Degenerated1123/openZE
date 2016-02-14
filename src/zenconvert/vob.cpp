#include <iostream>

#include "vob.h"

ZenConvert::Vob::Vob(const std::string &name, const std::string &className, int classVersion, Vob *pParent, int objectID) :
    m_Name(name),
    m_ClassName(className),
    m_ClassVersion(classVersion),
    m_ObjectID(objectID),
    m_pParent(pParent),
    m_pReference(nullptr)
{
}

ZenConvert::Vob::~Vob()
{
    for(auto pChild : m_Vobs)
        delete pChild;
    m_Vobs.clear();
}

ZenConvert::Vob *ZenConvert::Vob::addVob(const std::string &name, const std::string &className, int classVersion, int objectID, Vob *pReference)
{
    Vob *pChild = new Vob(name, className, classVersion, this, objectID);
    pChild->m_pReference = pReference;
    m_Vobs.push_back(pChild);
    return pChild;
}

void ZenConvert::Vob::addAttribute(const std::string &name, const std::string &value)
{
    if(m_Attributes.find(name) != m_Attributes.end())
        throw std::runtime_error("Same attribute was set earlier");

    m_Attributes.emplace(name, value);
}
