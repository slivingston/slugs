/*!
    \file    variableManager.cpp
    \brief   Variable manager for Slugs - provides the needed variable vectors
             and cubes

    --------------------------------------------------------------------------

    SLUGS: SmaLl bUt complete Gr(1) Synthesis tool

    Copyright (c) 2013-2014, Ruediger Ehlers, Vasumathi Raman, and Cameron Finucane
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in the
          documentation and/or other materials provided with the distribution.
        * Neither the name of any university at which development of this tool
          was performed nor the names of its contributors may be used to endorse
          or promote products derived from this software without specific prior
          written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

#include <map>
#include <set>
#include <string>

// Before loading "variableTypes", we define some classes and macros that build a list of variable
// types at the start of the program. These macros are then used in the "variableTypes.hpp" and thus
// build the required objects.
#define MERGENAMESSUB(X,Y) X##Y
#define MERGENAMES(X,Y) MERGENAMESSUB(X,Y)
#define INTERPRET_ADDITIONAL_VARIABLE_TYPE_INFO
#define REGISTER_VARIABLE_TYPE_STRING(a,b) RegisterVariableTypeClass MERGENAMES(variableTypeRegisterer,__LINE__)(a,b);
#define REGISTER_VARIABLE_TYPE_HIERARCHY(a,b) RegisterVariableHierarchyClass MERGENAMES(variableHierarchyRegisterer,__LINE__)(a,b);

// Static variable types map
std::map<std::string, int> *allPossibleVariableTypes = NULL;
class RegisterVariableTypeClass {
public:
    RegisterVariableTypeClass(int id, std::string name) {
        if (allPossibleVariableTypes==NULL) {
            allPossibleVariableTypes = new std::map<std::string, int>();
        }
        (*allPossibleVariableTypes)[name] = id;
    }
    ~RegisterVariableTypeClass() {
        if (allPossibleVariableTypes!=NULL) {
            delete allPossibleVariableTypes;
            allPossibleVariableTypes=0;
        }
    }
};

// Static variable hierarchy map
std::map<int, int> *variableHierarchy = NULL;
class RegisterVariableHierarchyClass {
public:
    RegisterVariableHierarchyClass(int object, int parent) {
        if (variableHierarchy==NULL) {
            variableHierarchy = new std::map<int, int>();
        }
        (*variableHierarchy)[object] = parent;
    }
    ~RegisterVariableHierarchyClass() {
        if (variableHierarchy!=NULL) {
            delete variableHierarchy;
            variableHierarchy=0;
        }
    }
};

// Only include the header files now so that the variable types and hierarchy information can
// be registered
#include "variableTypes.hpp"
#include "variableManager.hpp"


// =================================================
// Variable manager functions
// =================================================

void VariableManager::getVariableTypes(std::vector<std::string> &types) const {
    for (auto it = allPossibleVariableTypes->begin();it!=allPossibleVariableTypes->end();it++) {
        types.push_back(it->first);
    }
}

void VariableManager::getVariableNumbersOfType(std::string typeString, std::vector<unsigned int> &nums) const {
    assert(allPossibleVariableTypes->count(typeString)>0);
    int typeNumber = allPossibleVariableTypes->at(typeString);
    for (unsigned int i=0;i<variables.size();i++) {
        if (variableTypes[i]==typeNumber) {
            nums.push_back(i);
        }
    }
}

int VariableManager::addVariable(VariableType type, std::string name) {
    int varNumber = variables.size();
    variables.push_back(mgr.newVariable());
    variableNames.push_back(name);
    variableTypes.push_back(type);
    return varNumber;
}

/**
 * @brief Computes the variable vectors and cubes needed to perform the rest of the computation.
 *        Obtains the information which variables and cubes to compute from the instantions of the "SlugsVarVector"
 *        and "SlugsVarCube" classes, which registered their instantiation with the VariableManager.
 */
void VariableManager::computeVariableInformation() {

    // Compute a type hierarchy closure of all variables
    std::set<int> variableTypesAll[variables.size()];
    for (unsigned int i=0;i < variables.size();i++) {
        VariableType type = variableTypes[i];
        while (type!=NoneVariableType) {
            variableTypesAll[i].insert(type);
            type = static_cast<VariableType>((*variableHierarchy).at(type));
        }
    }

    // Compute Variable Vectors
    for (auto it = varVectorsToConstruct.begin();it!=varVectorsToConstruct.end();it++) {
        std::vector<BF> varsInThisVector;
        for (auto it2 = it->first.begin();it2!=it->first.end();it2++) {
            for (unsigned int i=0;i<variables.size();i++) {
                if (variableTypesAll[i].count(*it2)>0) {
                    std::cerr << "Adding to var vector:" << i << std::endl;
                    varsInThisVector.push_back(variables[i]);
                }
            }
        }
        *(it->second) = mgr.computeVarVector(varsInThisVector);
    }

}
