#ifndef __EXTENSION_EXPLICIT_COUNTERSTRATEGY_HPP
#define __EXTENSION_EXPLICIT_COUNTERSTRATEGY_HPP

#include "gr1context.hpp"
#include <string>

/**
 * A class that computes an explicit state counterstrategy for an unrealizable specification
 */
template<class T> class XExtractExplicitCounterStrategy : public T {
protected:
    // New variables
    std::string outputFilename;

    // Inherited stuff used
    using T::mgr;
    using T::strategyDumpingData;
    using T::livenessGuarantees;
    using T::livenessAssumptions;
    using T::varVectorPre;
    using T::varVectorPost;
    using T::varCubePostOutput;
    using T::varCubePostInput;
    using T::varCubePreOutput;
    using T::varCubePreInput;
    using T::varCubePre;
    using T::safetyEnv;
    using T::safetySys;
    using T::winningPositions;
    using T::initEnv;
    using T::initSys;
    using T::preVars;
    using T::postVars;
    using T::variableTypes;
    using T::variables;
    using T::variableNames;
    using T::realizable;
    using T::determinize;

    XExtractExplicitCounterStrategy<T>(std::list<std::string> &filenames) : T(filenames) {
        if (filenames.size()==0) {
            outputFilename = "";
        } else {
            outputFilename = filenames.front();
            filenames.pop_front();
        }
    }

public:

/**
 * @brief Compute and print out (to stdout) an explicit-state counter strategy that is winning for
 *        the environment. The output is compatible with the old JTLV output of LTLMoP.
 *        This function requires that the unrealizability of the specification has already been
 *        detected and that the variables "strategyDumpingData" and
 *        "winningPositions" have been filled by the synthesis algorithm with meaningful data.
 * @param outputStream - Where the strategy shall be printed to.
 */
void execute() {
        T::execute();
        if (!realizable) {
            if (outputFilename=="") {
                computeAndPrintExplicitStateStrategy(std::cout);
            } else {
                std::ofstream of(outputFilename.c_str());
                if (of.fail()) {
                    SlugsException ex(false);
                    ex << "Error: Could not open output file'" << outputFilename << "\n";
                    throw ex;
                }
                computeAndPrintExplicitStateStrategy(of);
                if (of.fail()) {
                    SlugsException ex(false);
                    ex << "Error: Writing to output file'" << outputFilename << "failed. \n";
                    throw ex;
                }
                of.close();
            }
        }
}

    
void computeAndPrintExplicitStateStrategy(std::ostream &outputStream) {

    // We don't want any reordering from this point onwards, as
    // the BDD manipulations from this point onwards are 'kind of simple'.
    mgr.setAutomaticOptimisation(false);

    // List of states in existance so far. The first map
    // maps from a BF node pointer (for the pre variable valuation) and a goal
    // to a state number. The vector then contains the concrete valuation.
    std::map<std::pair<size_t, std::pair<unsigned int, unsigned int> >, unsigned int > lookupTableForPastStates;
    std::vector<BF> bfsUsedInTheLookupTable;
    std::list<std::pair<size_t, std::pair<unsigned int, unsigned int> > > todoList;

    

    // Prepare positional strategies for the individual goals
    std::vector<std::vector<BF> > positionalStrategiesForTheIndividualGoals(livenessAssumptions.size());
    for (unsigned int i=0;i<livenessAssumptions.size();i++) {
        //BF casesCovered = mgr.constantFalse();
        std::vector<BF> strategy(livenessGuarantees.size()+1);
        for (unsigned int j=0;j<livenessGuarantees.size()+1;j++) {
            strategy[j] = mgr.constantFalse();
        }
        for (auto it = strategyDumpingData.begin();it!=strategyDumpingData.end();it++) {
            if (boost::get<0>(*it) == i) {
                //Have to cover each guarantee (since the winning strategy depends on which guarantee is being pursued)
                //Essentially, the choice of which guarantee to pursue can be thought of as a system "move".
                //The environment always to chooses that prevent the appropriate guarantee.
                if (strategy[boost::get<1>(*it)].isFalse()) {
                    strategy[boost::get<1>(*it)] = boost::get<2>(*it);
                }
            }
        }
        
        positionalStrategiesForTheIndividualGoals[i] = strategy;
        //BF_newDumpDot(*this,strategy,"PreInput PreOutput PostInput PostOutput","/tmp/generalStrategy.dot");
    }
    
    // Prepare initial to-do list from the allowed initial states. Select a single initial input valuation.
    BF todoInit = (winningPositions & initEnv & initSys);
    while (!(todoInit.isFalse())) {
        BF concreteState = determinize(todoInit,preVars);
        
        //find which liveness guarantee is being prevented (finds the first liveness in order specified)
        unsigned int found_j_index = 0;
        for (unsigned int j=0;j<livenessGuarantees.size();j++) {
            if (!(concreteState & !livenessGuarantees[j] & positionalStrategiesForTheIndividualGoals[0][j]).isFalse()) {
                found_j_index = j;
                break;
            }
        }
            
        std::pair<size_t, std::pair<unsigned int, unsigned int> > lookup = std::pair<size_t, std::pair<unsigned int, unsigned int> >(concreteState.getHashCode(),std::pair<unsigned int, unsigned int>(0,found_j_index));
        lookupTableForPastStates[lookup] = bfsUsedInTheLookupTable.size();
        bfsUsedInTheLookupTable.push_back(concreteState);
        //from now on use the same initial input valuation (but consider all other initial output valuations)
        todoInit &= concreteState.ExistAbstract(varCubePreOutput) & !concreteState;
        todoList.push_back(lookup);
    }

    // Extract strategy
    while (todoList.size()>0) {
        std::pair<size_t, std::pair<unsigned int, unsigned int> > current = todoList.front();
        todoList.pop_front();
        unsigned int stateNum = lookupTableForPastStates[current];
        BF currentPossibilities = bfsUsedInTheLookupTable[stateNum];
        // Print state information
        outputStream << "State " << stateNum << " with rank (" << current.second.first << "," << current.second.second << ") -> <";
        bool first = true;
        for (unsigned int i=0;i<variables.size();i++) {
            if (variableTypes[i] < PostInput) {
                if (first) {
                    first = false;
                } else {
                    outputStream << ", ";
                }
                outputStream << variableNames[i] << ":";
                outputStream << (((currentPossibilities & variables[i]).isFalse())?"0":"1");
            }
        }

        outputStream << ">\n\tWith successors : ";
        first = true;      
        
        currentPossibilities &= positionalStrategiesForTheIndividualGoals[current.second.first][current.second.second];
        
        BF remainingTransitions = ((currentPossibilities & safetySys).ExistAbstract(varCubePre));
        
        //check if there exists inputs that force a safety violation in the next time step
        if ((remainingTransitions & safetySys).isFalse() | !(remainingTransitions & ((mgr.constantFalse())).UnivAbstract(varCubePostOutput).ExistAbstract(varCubePostInput)).isFalse()) {
                addDeadlocked(remainingTransitions & ((mgr.constantFalse())).UnivAbstract(varCubePostOutput).ExistAbstract(varCubePostInput), current, bfsUsedInTheLookupTable,  lookupTableForPastStates, outputStream);
        } else {
        
        BF inputCaptured = mgr.constantTrue();
    
            
        // Switching goals
        while (!(remainingTransitions & safetySys).isFalse()) {
            
            BF newCombination;
             
            if ((remainingTransitions & livenessAssumptions[current.second.first]).isFalse()) {
                newCombination = determinize(remainingTransitions & safetySys, postVars);
            } else {
                newCombination = determinize((remainingTransitions & safetySys & livenessAssumptions[current.second.first]),postVars);          
            }
            // Jump as much forward  in the liveness assumption list as possible ("stuttering avoidance")
            unsigned int nextLivenessAssumption = current.second.first;
            bool firstTry = true;
            while (((nextLivenessAssumption != current.second.first) | firstTry) && !((livenessAssumptions[nextLivenessAssumption] & newCombination).isFalse())) {
                nextLivenessAssumption  = (nextLivenessAssumption + 1) % livenessAssumptions.size();
                firstTry = false;
            }
            
            

            //Mark which input has been captured by this case. Use the same input for other successors
            inputCaptured = newCombination.ExistAbstract(varCubePostOutput);
            remainingTransitions &= inputCaptured;
            remainingTransitions &= !newCombination;
           
            newCombination = newCombination.SwapVariables(varVectorPre,varVectorPost);
            
            if (nextLivenessAssumption != current.second.second) {
                unsigned int found_j_index = 0;
                for (unsigned int j=0;j<livenessGuarantees.size();j++) {
                    if (!(newCombination & !livenessGuarantees[j] & positionalStrategiesForTheIndividualGoals[current.second.first][j]).isFalse()) {
                        found_j_index = j;
                        break;
                    }
                }
                current.second.second = found_j_index;
            }
            
            unsigned int tn;
            
            std::pair<size_t, std::pair<unsigned int, unsigned int> > target;
            
            target = std::pair<size_t, std::pair<unsigned int, unsigned int> >(newCombination.getHashCode(),std::pair<unsigned int, unsigned int>(nextLivenessAssumption, current.second.second));
            
            if (lookupTableForPastStates.count(target)==0) {
                tn = lookupTableForPastStates[target] = bfsUsedInTheLookupTable.size();
                bfsUsedInTheLookupTable.push_back(newCombination);
                todoList.push_back(target);
            } else {
                tn = lookupTableForPastStates[target];
            }

            // Print                       
            if (first) {
                first = false;
            } else {
                outputStream << ", ";
            }
            outputStream << tn;
            
        }
        }

        outputStream << "\n";
    }
}

    //This function adds a new successor-less "state" that captures the deadlock-causing input values
    //The outputvalues are omitted (indeed, no valuation exists that satisfies the system safeties)
    //Format compatible with JTLV counterstrategy

void addDeadlocked(BF remainingTransitions, std::pair<size_t, std::pair<unsigned int, unsigned int> > current, std::vector<BF> &bfsUsedInTheLookupTable, std::map<std::pair<size_t, std::pair<unsigned int, unsigned int> >, unsigned int > &lookupTableForPastStates, std::ostream &outputStream) {
    BF newCombination;
                
    if ((remainingTransitions & livenessAssumptions[current.second.first]).isFalse()) {
        newCombination = determinize(remainingTransitions,postVars) ;
    } else {
        newCombination = determinize((remainingTransitions & livenessAssumptions[current.second.first]),postVars) ;           
    }
    newCombination = newCombination.SwapVariables(varVectorPre,varVectorPost);
    newCombination  = newCombination.UnivAbstract(varCubePreInput);         
    
    std::pair<size_t, std::pair<unsigned int, unsigned int> > target = std::pair<size_t, std::pair<unsigned int, unsigned int> >(newCombination.getHashCode(),std::pair<unsigned int, unsigned int>(current.second.first, current.second.second));
    unsigned int tn;
    
    if (lookupTableForPastStates.count(target)==0) {
        tn = lookupTableForPastStates[target] = bfsUsedInTheLookupTable.size();
        bfsUsedInTheLookupTable.push_back(newCombination);   
        outputStream << tn << "\n";
        
        //Note that since we are printing here, we usually end up with the states being printed out of order.
        //TOTO: print in order
        outputStream << "State " << tn << " with rank (" << current.second.first << "," << current.second.second << ") -> <";
        bool first = true;
        for (unsigned int i=0;i<variables.size();i++) {
            if (variableTypes[i] < PreOutput) {
                if (first) {
                    first = false;
                } else {
                    outputStream << ", ";
                }
                outputStream << variableNames[i] << ":";
                outputStream << (((newCombination & variables[i]).isFalse())?"0":"1");
            }
        }
        outputStream << ">\n\tWith no successors.";
    
    } else {
        tn = lookupTableForPastStates[target];
        outputStream << tn;
    }
    
}


    static GR1Context* makeInstance(std::list<std::string> &filenames) {
        return new XExtractExplicitCounterStrategy<T>(filenames);
    }
};

#endif
