#include "scheme_eval_apply.hpp"
using namespace Scheme;

void test1() {
    fmt::print("[ {} ]\n", __FUNCTION__);
    auto global = std::make_shared<Frame>();
    global->define("a", "221412421");
    global->define("b", true);
    global->define("c", 1.0);
    fmt::print("{}\n", eval(Value("a"), global));
    fmt::print("{}\n", eval(Value("b"), global));
    fmt::print("{}\n", eval(Value("c"), global));
    fmt::print("{}\n", eval(Value(1.22), global));
}


int main() {
    test1();
    return 0;
}