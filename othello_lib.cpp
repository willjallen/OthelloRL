// Build command cl /D_USRDLL /D_WINDLL test.cpp /MT /link /DLL /OUT:testDLL.dll

// If i were to **hypothetically** rewrite this, I would have multiple 64xint arrays and do the legal moves and flipping tiles all in one go

extern "C" {

    const int EMPTY = 0;
    const int PLAYER_1 = 1;
    const int PLAYER_2 = 2;
    // const int PLAYER_1_CHECKED = 3;
    // const int PLAYER_2_CHECKED = 4;
    const int LEGAL = 8;


    const int NORTH = 0;
    const int NORTHEAST = 1;
    const int EAST = 2;
    const int SOUTHEAST = 3;
    const int SOUTH = 4;
    const int SOUTHWEST = 5;
    const int WEST = 6;
    const int NORTHWEST = 7;

    const int DIRECTIONS[8] = {NORTH, NORTHEAST, EAST, SOUTHEAST, SOUTH, SOUTHWEST, WEST, NORTHWEST};

    const int DIRECTION_COMPONENTS[8][2] = {
            {0, 1}, // N = (x+0, y+1)
            {1, 1}, // NE = (x+1, y+1)
            {1, 0}, // E = (x+1, y+0)
            {1, -1}, // SE = (x+1, y-1)
            {0, -1}, // S = (x+0, y-1)
            {-1, -1}, // SW = (x-1, y-1)
            {-1, 0}, // W = (x-1, y+0)
            {-1, 1}, // NW = (x-1, y+1)
        };

    __declspec(dllexport)
    struct MoveLine {
        unsigned int row;
        unsigned int col;

        unsigned int direction;
        unsigned int length;
    };

    /**
     * @brief A structure containing all relevant game information for a particular state
     * 
     */
    __declspec(dllexport)    
    struct GameState {
        unsigned int numBlackTiles;
        unsigned int numWhiteTiles;

        unsigned int numBlackLegalMoves;
        unsigned int numWhiteLegalMoves;

        unsigned int board[8][8];
        unsigned int legalMoves[8][8];

        MoveLine* moveLines;

    };
    
    __declspec(dllexport)
    // void initGame() {

    // }


    /**
     * @brief Checks whether given coordinates are within the board
     * 
     * @param i 
     * @param j 
     * @return true 
     * @return false 
     */
    __declspec(dllexport)
    bool inBounds(int x, int y){
        if(0 <= x && x < 8 && 0 <= y && y < 8 ){
            return true;
        }else{
            return false;
        }
    } 

    __declspec(dllexport)
    void setLegalMove(int board[8][8], int startingX, int startingY, int player, int otherPlayer){
        
        // For all 8 cardinal directions
        for(int direction = 0; direction < 8; direction++){

            int dx = DIRECTION_COMPONENTS[direction][0];
            int dy = DIRECTION_COMPONENTS[direction][1];

            // Check if the first adjacent cell is in bounds, and if it is whether it is the other player
            if(inBounds(startingX + dx, startingY + dy)){
                if(board[startingX + dx][startingY + dy] != otherPlayer) continue;
            }else{
                continue;
            }

            // Start at the adjacent cell
            int x = startingX + dx;
            int y = startingY + dy;
            

            // Continue until we reach the end of the board
            while(inBounds(x + dx, y + dy)){

                // Already legal
                if(board[x][y] == LEGAL) goto beach;


                // In the case that a line is already flanked on both sides by a chip (BWWB), we can not place a B chip like (BWWBB)
                if(board[x][y] == player) goto beach;

                // Found legal move
                if(board[x][y] == EMPTY){
                    board[x][y] = LEGAL;
                    goto beach;
                }

                // Keep searching
                if(board[x][y] == otherPlayer){
                    x += dx;
                    y += dy;
                }



            }

            // If we reached the end of the board, and the cell is empty, set it to legal
            if(board[x][y] == EMPTY){
                board[x][y] = LEGAL;
            }
        
            beach: continue;

        }

        


    }


    /**
     * @brief Calculates and returns and 8x8 array of legal positions   
     * 
     * @param board 8x8 board where 0 = empty, 1 = player 1, 2 = player 2
     * @param player 1 = player 1, 2 = player 2
     * @return int** 
     */
    __declspec(dllexport)
    void calculateLegalMoves(int board[8][8], int player){

        int otherPlayer = 0;
        if(player == 1){
            otherPlayer = 2;
        }else{
            otherPlayer = 1;
        }
        
        // Clear previous legal moves
        for(int x = 0; x < 8; x++){
            for(int y = 0; y < 8; y++){
                if(board[x][y] == LEGAL) board[x][y] = EMPTY;
            }
        }


        for(int x = 0; x < 8; x++){
            for(int y = 0; y < 8; y++){
                int boardVal = board[x][y];

                if(boardVal == EMPTY) continue;

                if(boardVal == player){
                    // Check the 8 cardinal directions, do not wrap around the board
                    setLegalMove(board, x, y, player, otherPlayer);
                }

            }
        }

        // return board;
    }

    __declspec(dllexport)
    void playMove(int board[8][8], int row, int col, int player){
       
        int otherPlayer = -1;
        if(player == 1){
            otherPlayer = 2;
        }else{
            otherPlayer = 1;
        }

        // Set current tile
        board[row][col] = player;

        
        int* flipArr[64];
        for(int i = 0; i < 64; i++){
            flipArr[i] = nullptr;
        }
        bool flip[8];
        for(int i = 0; i < 8; i++){
            flip[i] = false;
        }

        // For all 8 cardinal directions
        for(int direction = 0; direction < 8; direction++){

            int x = row;
            int y = col;

            int dx = DIRECTION_COMPONENTS[direction][0];
            int dy = DIRECTION_COMPONENTS[direction][1];            

            // Continue until we reach the end of the board or an empty cell
            int idx = 8*direction;
            while(inBounds(x + dx, y + dy)){
                x += dx;
                y += dy;

                if(board[x][y] == EMPTY || board[x][y] == LEGAL){
                    flip[direction] = false;
                    goto beach;
                }
                
                flipArr[idx] = &(board[x][y]);
                idx++;

                if(board[x][y] == player){
                    flip[direction] = true;
                    goto beach;
                }
            }

            beach: continue;
        }

        // Flip tiles
        for(int i = 0; i < 8; i++){
            if(flip[i]){
                for(int j = i*8; j < (i*8)+8; j++){
                    if(flipArr[j] == nullptr) break;
                    *(flipArr[j]) = player;
                }
            }
        }
    }


}