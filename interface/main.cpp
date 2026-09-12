#include <SFML/Graphics.hpp>
#include <iostream>

#define BOARD_SIDE_SIZE 800


class ChessBoard {
public:
    sf::RectangleShape board[8][8];

    ChessBoard(){

        for ( int i=0; i < 8; i++){

            for ( int j=0; j < 8; j++){
                board[i][j].setSize({BOARD_SIDE_SIZE/8, BOARD_SIDE_SIZE/8});
                
                std::cout << i*i << "\n";

                if( (i+j) % 2 == 0){    
                    board[i][j].setFillColor(sf::Color(255, 255, 255));

                }

                else{
                    board[i][j].setFillColor(sf::Color(0, 0, 0));

                }

                board[i][j].setPosition({ i * (BOARD_SIDE_SIZE / 8.f) , j * (BOARD_SIDE_SIZE / 8.f)});
            }
        }
    }
};

int main()
{
    // create the window
    sf::RenderWindow window(sf::VideoMode({BOARD_SIDE_SIZE, BOARD_SIDE_SIZE}), "My window");

    ChessBoard board;

    
    // run the program as long as the window is open
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        while (const std::optional event = window.pollEvent())
        {
            // "close requested" event: we close the window
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        // clear the window with black color
        window.clear(sf::Color::Black);

        // draw everything here...

        for ( int i=0; i < 8; i++){

            for ( int j=0; j < 8; j++){
                window.draw(board.board[i][j]);
            }
        }
       

        // end the current frame
        window.display();
    }
}
