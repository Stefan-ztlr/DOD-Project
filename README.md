# DOD Project

# Breakout Game

A breakout clone that implements a multi threaded spatial grid and an ECS system to try and simulate a high number of entities, in this case balls.

# Game Mechanics
- destroying a brick can cause a powerup to drop, once picked up it doubles the amount of balls on the screen
- clearing an entire level of bricks makes you go to the next one where the number of games increases (level - 1 game, level 2 - 4, level 3 - 9,..., level n- nxn)
- currently might break at higher levels 
