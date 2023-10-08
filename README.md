# cpp-reflect

Language-level reflection for C++ has a long and uncertain road ahead of it, and it appears less and less work being put into it ([https://lists.isocpp.org/sg7/]).

This project was an effort to provide build tooling for embedding Clang AST information about source code into the built binary, and then provide APIs for accessing the AST information from within that program at runtime.

## Status

Partial proof of concept functional, abandoned.

## Related work

Beyond interesting work on successor languages, for C++ "reflection" there are some interesting projects worth checking out.

- [Neargye/magic_enum](https://github.com/Neargye/magic_enum) involves some clever use of compiler predefined macros such as `__PRETTY_FUNCTION__`, expanding and parsing at compile time in order to get compile time reflection information.  This technique could probably be extended more generally to functions etc., likely with more limitations between different compilers though.
- [felixguendling/cista](https://github.com/felixguendling/cista) does struct [reflection](https://cista.rocks/#reflection) for implementing "automatic" serialization.  This technique relies on some useful properties of C++17 structured bindings, plus some clever tricks as originally (I think) described [in this blog post](https://playfulprogramming.blogspot.com/2016/12/serializing-structs-with-c17-structured.html).
