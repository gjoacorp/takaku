# takaku

## What is takaku?

takaku (多角) is a C++/[raylib](https://www.raylib.com) implementation of the abstract strategy game [Sim](https://en.wikipedia.org/wiki/Sim_(game)).

## Current features
### Play on boards of arbitrarily large size
As a consequence of [Ramsey's theorem](https://en.wikipedia.org/wiki/Ramsey%27s_theorem), the minimum number of vertices required to guarantee that a draw cannot occur in a game of Sim is 6. Thus, Sim is typically played on a size 6 board (i.e. featuring six vertices). However, mathematically there is nothing preventing Sim from being played on a board of size $n \geq 6$, which takaku supports.

### Play against humans or the computer
Currently, takaku supports playing against a human locally or playing against the computer. However, the computer's AI implementation is somewhat primitive, though not easy to beat!

### Interact with a dynamic board

Traditionally, Sim is played on a piece of paper on which the vertices are obviously fixed for the duration of the game (i.e. a static board). takaku chooses to take a different approach by allowing the board's vertices to be moved around at will by the player. 

## Potential future features

- Peer-to-peer multiplayer support.
- Variant game modes.
- Customisable themes.

## Current state of development

I was in a bit of a daze when I created takaku (I was midway through completing my master's degree in mathematics) and, as a result, I'm not entirely happy with the state of the codebase. Hence, the current focus is on repaying some technical debt (refactoring) and not adding new features.

## Making contributions

I would be overjoyed if anyone wants to help assist me with the development of this project. The best way to help out would be to look for refactoring opportunities within the codebase and submit pull requests for review. If you have any questions, please feel free to submit an issue.
