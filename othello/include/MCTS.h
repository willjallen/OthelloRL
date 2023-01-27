#ifndef MCTS_H
#define MCTS_H

#include <limits.h>
#include <random>
#include "Othello.h"
#include "StateSearchTree.h"


class MCTS {
  
  public:
    MCTS(Othello::GameState &initialState);
   ~MCTS(); 

    float search(Othello::GameState &gameState);
    std::vector<std::pair<std::pair<unsigned int, unsigned int>, float>> getPI(Othello::GameState &gameState);

    
    StateSearchTree *stateSearchTree;
  private:

};


#endif