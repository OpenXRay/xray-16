===========================================================
================ ScRaceSample Overview ====================
===========================================================

The ScRaceSample program is intended to give developers an
example for how the Competition SDK is used in practice. The
sample game is a simple 1v1 racing game using a
challenge-response mechanism.

==========
HOW TO USE
==========
1. Login with a GameSpy account

When starting the sample, you will first see a Dialog Box
prompting you to login. Using your GameSpy account, login with
your e-mail, profile nickname and password. If you do not have
an account, you can create a free account at www.gamespyid.com.

2. Choose to Host or Join a game

After the login is complete, another Dialog Box will ask if you
plan to Host or Join the game. Once another player has a game
being hosted, you can enter that IP in order to join into his/her
game. This sample is very basic and does not do any NAT
Negotiation for Hosts behind NATs, so make sure ports are open for
connections that drop unsolicited packets. 

The Sample uses a default port of 38466 for the Host, which can be
modified at the top of ScRaceSampleDlg.cpp by changing the value
of HOST_PORT_STRING. The CLIENT_PORT_STRING is set to 38467 to
allow for local joining of a session.

3. Start the Game

Once the two players are connected. The Host can start the game
by clicking on "Start Game". The game will countdown from 5 and
say "GO!" when the game begins. To race, players must alternate
between pressing 'Z' and 'X' on the keyboard as quickly as
possible. The progress bar will display how far you and your
opponent are from the finish. The game will wait until both 
players have finished before displaying the results.