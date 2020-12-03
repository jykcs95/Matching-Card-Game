# Matching-Card-Game
Final Project in Operating Systems class - worked with Alex Ralle
The project implements a simple “matching game” using a server and up to five other clients. The  server  will  execute  in  one  of  the  osnodes  and each  client  on different osnodes. 

Rules for playing the “matching game”:
1.Mix up the cards.
2.Lay them in rows and columns, face down.
3.A player turnsover any two cards which can be seen by every player in the game.
4.If the two cards match, the player who turned them over keepsthe cards and the cards are removed from the game.
5.If they don’t match, the cards are turned over and stay in the game.
6.Each player should (obviously) try to remember the content and location of each card that remains in play. 
