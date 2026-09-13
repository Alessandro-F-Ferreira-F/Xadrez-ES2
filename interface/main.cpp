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

// class Piece{

// }

int main()
{
    // create the window
    sf::RenderWindow window(sf::VideoMode({BOARD_SIDE_SIZE, BOARD_SIDE_SIZE}), "My window");

    ChessBoard board;

    sf::Texture texture("./assets/pieces/w-pawn.png"); // Throws sf::Exception if an error occurs
    sf::Sprite sprite(texture);

    float squareSize = BOARD_SIDE_SIZE / 8.f;
    // 3. Fator de escala proporcional
    // Exemplo: se a imagem tem 480px e a casa tem 100px -> 100.f / 480.f ≈ 0.208
    float scaleX = squareSize / texture.getSize().x;
    float scaleY = squareSize / texture.getSize().y;

    sprite.setScale({scaleX, scaleY});
    texture.setSmooth(true);

    
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


        for ( int i=0; i < 8; i++){

            for ( int j=0; j < 8; j++){
                window.draw(board.board[i][j]);
            }
        }

        // inside the main loop, between window.clear() and window.display()
        window.draw(sprite);
       

        // end the current frame
        window.display();
    }
}
