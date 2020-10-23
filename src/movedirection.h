#ifndef MOVEDIRECTION
#define MOVEDIRECTION

/// What directory action are we executing
enum class MoveDirection : unsigned int {
    descend,
    ascend,
    list
};

#endif // MOVEDIRECTION

