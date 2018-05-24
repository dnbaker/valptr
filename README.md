# valptr
Stores a pointer and a value using unused bits in the pointer

This is like a `unique_ptr`, taking a general functor to perform the deletion as necessary,
while holding an integral value in the remaining bits.
A char pointer will have up to 16 bits available, while a pointer to a 64-bit integer will have 19.
This depends on `alignof(T)`.

The primary purpose is associating a count with a pointer without paying any additional space or finding values elsewhere.
