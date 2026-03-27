#include "scheme.hpp"
int main() {
    using namespace Scheme;
    Scheme::Pair p(1, Pair(2, Pair(3, nil)));
    fmt::print("{}\n", p);
}